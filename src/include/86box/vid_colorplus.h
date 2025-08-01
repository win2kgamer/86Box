#ifndef VIDEO_COLORPLUS_H
#define VIDEO_COLORPLUS_H

typedef struct colorplus_t {
    cga_t   cga;
    uint8_t control;

    lpt_t * lpt;
} colorplus_t;

void    colorplus_init(colorplus_t *colorplus);
void    colorplus_out(uint16_t addr, uint8_t val, void *priv);
uint8_t colorplus_in(uint16_t addr, void *priv);
void    colorplus_write(uint32_t addr, uint8_t val, void *priv);
uint8_t colorplus_read(uint32_t addr, void *priv);
void    colorplus_recalctimings(colorplus_t *colorplus);
void    colorplus_poll(void *priv);

extern const device_t colorplus_device;

#endif /*VIDEO_COLORPLUS_H*/
