// SPDX-License-Identifier: GPL-2.0-or-later

#include "zp.h"

#include <stdlib.h>

static unsigned short m_table[256] = {0}; // FIXME
static unsigned short p_table[256] = {0}; // FIXME
static Context up_table[256] = {0}; // FIXME
static Context dn_table[256] = {0}; // FIXME

struct Codec {
	unsigned char byte;
	unsigned char scount;
	unsigned char delay;
	unsigned int  a;
	unsigned int  code;
	unsigned int  fence;
	unsigned int  subend;
	unsigned int  buffer;
	unsigned int  nrun;

	char (*get)(void *);
	void (*put)(char, void *);
	void *context;
};

struct Codec *zp_new_encoder(void) {
	struct Codec *codec = malloc(sizeof(struct Codec));
	if (!codec) {
		abort();
	}
	codec->a = 0;
	codec->scount = 0;
	codec->byte = 0;
	codec->delay = 25;
	codec->subend = 0;
	codec->buffer = 0xffffff;
	codec->nrun = 0;
	return codec;
}

static void outbit(struct Codec *encoder, int bit) {
	if (encoder->delay > 0) {
		if (encoder->delay < 0xff) {
			encoder->delay -= 1;
		}
	} else {
		encoder->byte = (encoder->byte << 1) | bit;
		if (++encoder->scount == 8) {
			encoder->put(encoder->byte, encoder->context);
		}
		encoder->scount = 0;
		encoder->byte = 0;
	}
}

static void zemit(struct Codec *encoder, int b) {
	encoder->buffer = (encoder->buffer << 1) + b;
	b = encoder->buffer >> 24;
	encoder->buffer &= 0xffffff;
	switch (b) {
	case 1:
		outbit(encoder, 1);
		while (encoder->nrun-- > 0) {
			outbit(encoder, 0);
		}
		encoder->nrun = 0;
		break;
	case 0xff:
		outbit(encoder, 0);
		while (encoder->nrun-- > 0) {
			outbit(encoder, 1);
		}
		encoder->nrun = 0;
		break;
	case 0:
		encoder->nrun += 1;
		break;
	default:
		abort();
	}
}

void zp_flush(struct Codec *encoder) {
	if (encoder->subend > 0x8000) {
		encoder->subend = 0x10000;
	} else if (encoder->subend == 0) {
		encoder->subend = 0x8000;
	}
	while (encoder->buffer != 0xffffff || encoder->subend) {
		zemit(encoder, 1 - (encoder->subend >> 15));
		encoder->subend = (unsigned short)(encoder->subend << 1);
	}
	outbit(encoder, 1);
	while (encoder->nrun-- > 0) {
		outbit(encoder, 0);
	}
	encoder->nrun = 0;
	while (encoder->scount > 0) {
		outbit(encoder, 1);
	}
	encoder->delay = 0xff;
}

static void encode_mps(struct Codec *encoder, Context *ctx, unsigned int z) {
	unsigned int d = 0x6000 + ((z + encoder->a) >> 2);
	if (z > d) {
		z = d;
	}
	if (encoder->a >= m_table[*ctx]) {
		*ctx = up_table[*ctx];
	}
	encoder->a = z;
	if (encoder->a >= 0x8000) {
		zemit(encoder, 1 - (encoder->subend >> 15));
		encoder->subend = (unsigned short)(encoder->subend << 1);
		encoder->a = (unsigned short)(encoder->a << 1);
	}
}

static void encode_lps(struct Codec *encoder, Context *ctx, unsigned int z) {
	unsigned int d = 0x6000 + ((z + encoder->a) >> 2);
	if (z > d) {
		z = d;
	}
	*ctx = dn_table[*ctx];
	z = 0x10000 - z;
	encoder->subend += z;
	encoder->a += z;
	while (encoder->a >= 0x8000) {
		zemit(encoder, 1 - (encoder->subend >> 15));
		encoder->subend = (unsigned short)(encoder->subend << 1);
		encoder->a = (unsigned short)(encoder->a << 1);
	}
}

static void encode_mps_simple(struct Codec *encoder, unsigned int z) {
	encoder->a = z;
	if (encoder->a >= 0x8000) {
		zemit(encoder, 1 - (encoder->subend >> 15));
		encoder->subend = (unsigned short)(encoder->subend << 1);
		encoder->a = (unsigned short)(encoder->a << 1);
	}
}

static void encode_lps_simple(struct Codec *encoder, unsigned int z) {
	z = 0x10000 - z;
	encoder->subend += z;
	encoder->a += z;
	while (encoder->a >= 0x8000) {
		zemit(encoder, 1 - (encoder->subend >> 15));
		encoder->subend = (unsigned short)(encoder->subend << 1);
		encoder->a = (unsigned short)(encoder->a << 1);
	}
}

void zp_encode(struct Codec *encoder, int bit, Context *ctx) {
	unsigned int z = encoder->a + p_table[*ctx];
	if (bit != (*ctx & 1)) {
		encode_lps(encoder, ctx, z);
	} else if (z >= 0x8000) {
		encode_mps(encoder, ctx, z);
	} else {
		encoder->a = z;
	}
}

void zp_encode_passthrough(struct Codec *encoder, int bit) {
	if (bit) {
		encode_lps_simple(encoder, 0x8000 + (encoder->a >> 1));
	} else {
		encode_mps_simple(encoder, 0x8000 + (encoder->a >> 1));
	}
}

struct Codec *zp_new_decoder(void) {
	abort(); // TODO
}

void zp_codec_delete(struct Codec *codec) {
	free(codec);
}
