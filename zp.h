// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef ZP_H
#define ZP_H

struct Codec;
typedef unsigned char Context;

struct Codec *zp_new_encoder(void);
void zp_encode(struct Codec *, int, Context *);
void zp_encode_passthrough(struct Codec *, int);
void zp_flush(struct Codec *);

struct Codec *zp_new_decoder(void);

void zp_codec_delete(struct Codec *);

#endif
