/***************************************************************************
 *   Copyright (C) 2009 by Sindre Aamås                                    *
 *   sinamas@users.sourceforge.net                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License version 2 for more details.                *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   version 2 along with this program; if not, write to the               *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "rgb32conv.h"
#include "videolink.h"
#include "array.h"
#include "gbint.h"
#include <algorithm>

namespace {

static bool isBigEndian() {
	union {
		gambatte::uint_least32_t ul32;
		unsigned char uc[sizeof(gambatte::uint_least32_t)];
	} u;
	u.ul32 = -0x10000;
	return u.uc[0];
}

class Rgb32ToUyvy {
	struct CacheUnit {
		gambatte::uint_least32_t rgb32;
		gambatte::uint_least32_t uyvy;
	};

	enum { cache_size = 0x100 };
	enum { cache_mask = cache_size - 1 };

	CacheUnit cache[cache_size];

public:
	Rgb32ToUyvy();
	void operator()(const gambatte::uint_least32_t *s, gambatte::uint_least32_t *d,
	                unsigned w, unsigned h, std::ptrdiff_t srcPitch, std::ptrdiff_t dstPitch);
};

Rgb32ToUyvy::Rgb32ToUyvy() {
	if (isBigEndian()) {
		CacheUnit c = { 0, 128ul << 24 | 16ul << 16 | 128 << 8 | 16 };
		std::fill(cache, cache + cache_size, c);
	} else {
		CacheUnit c = { 0, 16ul << 24 | 128ul << 16 | 16 << 8 | 128 };
		std::fill(cache, cache + cache_size, c);
	}
}

void Rgb32ToUyvy::operator()(const gambatte::uint_least32_t *s, gambatte::uint_least32_t *d,
		const unsigned w, unsigned h, const std::ptrdiff_t s_pitch, const std::ptrdiff_t d_pitch)
{
	while (h--) {
		unsigned n = w >> 1;

		do {
			if ((cache[s[0] & cache_mask].rgb32 - s[0]) | (cache[s[1] & cache_mask].rgb32 - s[1])) {
				cache[s[0] & cache_mask].rgb32 = s[0];
				cache[s[1] & cache_mask].rgb32 = s[1];

				const unsigned long r = (s[0] >> 16 & 0x000000FF) | (s[1]       & 0x00FF0000);
				const unsigned long g = (s[0] >>  8 & 0x000000FF) | (s[1] <<  8 & 0x00FF0000);
				const unsigned long b = (s[0]       & 0x000000FF) | (s[1] << 16 & 0x00FF0000);

				const unsigned long y = r *  66 + g * 129 + b * 25 + ( 16 * 256 + 128) * 0x00010001ul;
				const unsigned long u = b * 112 - r *  38 - g * 74 + (128 * 256 + 128) * 0x00010001ul;
				const unsigned long v = r * 112 - g *  94 - b * 18 + (128 * 256 + 128) * 0x00010001ul;

				if (isBigEndian()) {
					cache[s[0] & cache_mask].uyvy = (u << 16 & 0xFF000000)
					                              | (y <<  8 & 0x00FF0000)
					                              | (v       & 0x0000FF00)
					                              | (y >>  8 & 0x000000FF);
					cache[s[1] & cache_mask].uyvy = (u       & 0xFF000000)
					                              | (y >>  8 & 0x00FF0000)
					                              | (v >> 16 & 0x0000FF00)
					                              |  y >> 24              ;
				} else {
					cache[s[0] & cache_mask].uyvy = (y << 16 & 0xFF000000)
					                              | (v <<  8 & 0x00FF0000)
					                              | (y       & 0x0000FF00)
					                              | (u >>  8 & 0x000000FF);
					cache[s[1] & cache_mask].uyvy = (y       & 0xFF000000)
					                              | (v >>  8 & 0x00FF0000)
					                              | (y >> 16 & 0x0000FF00)
					                              |  u >> 24              ;
				}
			}

			d[0] = cache[s[0] & cache_mask].uyvy;
			d[1] = cache[s[1] & cache_mask].uyvy;
			s += 2;
			d += 2;
		} while (--n);

		s += s_pitch - static_cast<std::ptrdiff_t>(w);
		d += d_pitch - static_cast<std::ptrdiff_t>(w);
	}
}

static void rgb32ToRgb16(const gambatte::uint_least32_t *s, gambatte::uint_least16_t *d,
		const unsigned w, unsigned h, const std::ptrdiff_t srcPitch, const std::ptrdiff_t dstPitch)
{
	do {
		std::ptrdiff_t i = -static_cast<std::ptrdiff_t>(w);
		s += w;
		d += w;

		do {
			d[i] = (s[i] >> 8 & 0xF800) | (s[i] & 0xFC00) >> 5 | (s[i] & 0xFF) >> 3;
		} while (++i);

		s += srcPitch - static_cast<std::ptrdiff_t>(w);
		d += dstPitch - static_cast<std::ptrdiff_t>(w);
	} while (--h);
}

class Rgb32ToUyvyLink : public VideoLink {
	const Array<gambatte::uint_least32_t> inbuf_;
	Rgb32ToUyvy rgb32ToUyvy;
	const unsigned width_;
	const unsigned height_;

public:
	Rgb32ToUyvyLink(unsigned width, unsigned height)
	: inbuf_(static_cast<std::size_t>(width) * height),
	  width_(width),
	  height_(height)
	{
	}

	virtual void* inBuf() const { return inbuf_; }
	virtual std::ptrdiff_t inPitch() const { return width_; }

	virtual void draw(void *dst, std::ptrdiff_t dstpitch) {
		rgb32ToUyvy(inbuf_, static_cast<gambatte::uint_least32_t*>(dst), width_, height_, inPitch(), dstpitch);
	}
};

class Rgb32ToRgb16Link : public VideoLink {
	const Array<gambatte::uint_least32_t> inbuf_;
	const unsigned width_;
	const unsigned height_;

public:
	Rgb32ToRgb16Link(unsigned width, unsigned height)
	: inbuf_(static_cast<std::size_t>(width) * height),
	  width_(width),
	  height_(height)
	{
	}

	virtual void* inBuf() const { return inbuf_; }
	virtual std::ptrdiff_t inPitch() const { return width_; }

	virtual void draw(void *dst, std::ptrdiff_t dstpitch) {
		rgb32ToRgb16(inbuf_, static_cast<gambatte::uint_least16_t*>(dst), width_, height_, inPitch(), dstpitch);
	}
};

} // anon namespace

VideoLink* Rgb32Conv::create(PixelFormat pf, unsigned width, unsigned height) {
	switch (pf) {
	case RGB16: return new Rgb32ToRgb16Link(width, height);
	case  UYVY: return new Rgb32ToUyvyLink(width, height);
	default: return 0;
	}
}
