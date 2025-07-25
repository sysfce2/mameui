// license:BSD-3-Clause
// copyright-holders:David Haywood
/***************************************************************************

Janshi / Pinkiri 8 / Ron Jan
 - Wing / Eagle Mahjong board using HD647180X0P6 with internal ROM


 Todo:
 - background tilemap is not fully understood, we lack evidence to support
   it properly, all the games here just do a solid fill of one tile!

 - sprite heights?!

============================================================================
Janshi
(c)1992 Eagle

CPU: HD647180X0P6 (16K EPROM internal rom)
Sound: AY-3-8910, M6295
Others: Battery
OSC: 32MHz, 21MHz & 12.2880MHz

ROMs:
1.1A         [92b140a5]
2.1B         [6de7e086]
3.1D         [4e94d8f2]
4.1F         [a5f6e3ef]
5.1H         [ff2cc769]
6.1K         [8197034d]
11.1L        [a7692ddf]



--- Team Japump!!! ---
Dumped by Chackn
04/May/2007

***************************************************************************/

#include "emu.h"

#include "cpu/z180/hd647180x.h"

#include "sound/okim6295.h"

#include "emupal.h"
#include "screen.h"
#include "speaker.h"
#include "tilemap.h"

#define LOG_VRAM (1U << 1)

#define VERBOSE (0)
//#define LOG_OUTPUT_FUNC printf
#include "logmacro.h"


/* VDP device to give us our own memory map */
class janshi_vdp_device : public device_t, public device_memory_interface, public device_gfx_interface
{
public:
	// constructor/destructor
	janshi_vdp_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// configurations
	void set_janshi_hack(bool janshi_hack) { m_janshi_hack = janshi_hack; }

	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void write(offs_t offset, uint8_t data);

protected:
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual void device_validity_check(validity_checker &valid) const override;
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;
	virtual space_config_vector memory_space_config() const override;

private:
	address_space_config m_space_config;

	required_device<palette_device> m_palette;

	required_shared_ptr<uint8_t> m_back_vram;
	required_shared_ptr<uint8_t> m_vram1;
	required_shared_ptr<uint8_t> m_unk1;
	required_shared_ptr<uint8_t> m_widthflags;
	required_shared_ptr<uint8_t> m_unk2;
	required_shared_ptr<uint8_t> m_vram2;
	required_shared_ptr<uint8_t> m_crtc_regs;

	// internal states
	tilemap_t *m_tilemap = nullptr;
	uint32_t m_vram_addr;
	int32_t m_prev_writes;

	// configurations
	bool m_janshi_hack;

	void back_vram_w(offs_t offset, uint8_t data);
	DECLARE_GFXDECODE_MEMBER(gfxinfo);
	TILE_GET_INFO_MEMBER(get_tile_info);

	void draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect);

	void map(address_map &map) ATTR_COLD;
};

class pinkiri8_state : public driver_device
{
public:
	pinkiri8_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_vdp(*this, "janshivdp"),
		m_io_pl{{*this, "PL1_%u", 1U}, {*this, "PL2_%u", 1U}}
	{ }

	void pinkiri8(machine_config &config);
	void ronjan(machine_config &config);
	void janshi(machine_config &config);

protected:
	virtual void machine_start() override ATTR_COLD;

private:
	void output_regs_w(uint8_t data);
	void io_matrix_w(uint8_t data);
	template <unsigned Player> uint8_t io_matrix_r();
	uint8_t ronjan_prot_r();
	void ronjan_prot_w(uint8_t data);
	uint8_t ronjan_prot_status_r();
	uint8_t ronjan_patched_prot_r();

	void pinkiri8_io(address_map &map) ATTR_COLD;
	void prg_map(address_map &map) ATTR_COLD;
	void ronjan_io(address_map &map) ATTR_COLD;

	required_device<hd647180x_device> m_maincpu;
	required_device<janshi_vdp_device> m_vdp;

	required_ioport_array<5> m_io_pl[2];

	uint8_t m_io_matrix = 0;
	uint8_t m_prot_read_index = 0;
	uint8_t m_prot_char[5]{};
	uint8_t m_prot_index = 0;
};



void janshi_vdp_device::map(address_map &map)
{
	map(0xfc0000, 0xfc1fff).ram().w(FUNC(janshi_vdp_device::back_vram_w)).share(m_back_vram); // bg tilemap?
	map(0xfc2000, 0xfc2fff).ram().share(m_vram1); // xpos, colour, tile number etc.

	map(0xfc3700, 0xfc377f).ram().share(m_unk1); // ?? height related?
	map(0xfc3780, 0xfc37bf).ram().share(m_widthflags);
	map(0xfc37c0, 0xfc37ff).ram().share(m_unk2); // 2x increasing tables 00 10 20 30 etc.

	map(0xfc3800, 0xfc3fff).ram().share(m_vram2); // y pos + unknown

	map(0xff0000, 0xff07ff).ram().w(m_palette, FUNC(palette_device::write8)).share("palette"); //ram().w(FUNC(janshi_vdp_device::paletteram_xBBBBBGGGGGRRRRR_byte_split_lo_w));
	map(0xff2000, 0xff27ff).ram().w(m_palette, FUNC(palette_device::write8_ext)).share("palette_ext"); //ram().w(FUNC(janshi_vdp_device::paletteram_xBBBBBGGGGGRRRRR_byte_split_hi_w));

	map(0xff6000, 0xff601f).ram().share(m_crtc_regs);
}

DEFINE_DEVICE_TYPE(JANSHIVDP, janshi_vdp_device, "janshi_vdp", "Janshi VDP")

janshi_vdp_device::janshi_vdp_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, JANSHIVDP, tag, owner, clock)
	, device_memory_interface(mconfig, *this)
	, device_gfx_interface(mconfig, *this, gfxinfo, "palette")
	, m_space_config("janshi_vdp", ENDIANNESS_LITTLE, 8,24, 0, address_map_constructor(FUNC(janshi_vdp_device::map), this))
	, m_palette(*this, "palette")
	, m_back_vram(*this, "back_vram")
	, m_vram1(*this, "vram1")
	, m_unk1(*this, "unk1")
	, m_widthflags(*this, "widthflags")
	, m_unk2(*this, "unk2")
	, m_vram2(*this, "vram2")
	, m_crtc_regs(*this, "crtc_regs")
	, m_vram_addr(0)
	, m_prev_writes(0)
	, m_janshi_hack(false)
{
}

static const gfx_layout charlayout =
{
	16,8,
	RGN_FRAC(1,5),
	5,
	{ RGN_FRAC(4,5),RGN_FRAC(3,5),RGN_FRAC(2,5),RGN_FRAC(1,5),RGN_FRAC(0,5) },
	{ STEP8(0,1), STEP8(8*8,1) },
	{ STEP8(0,8) },
	8*16
};

GFXDECODE_MEMBER(janshi_vdp_device::gfxinfo)
	GFXDECODE_DEVICE(DEVICE_SELF, 0, charlayout, 0, 0x40 )
GFXDECODE_END

void janshi_vdp_device::device_add_mconfig(machine_config &config)
{
	PALETTE(config, m_palette).set_format(palette_device::xBGR_555, 0x800);
}

void janshi_vdp_device::device_validity_check(validity_checker &valid) const {}

void janshi_vdp_device::device_start()
{
	m_tilemap = &machine().tilemap().create(*this, tilemap_get_info_delegate(*this, FUNC(janshi_vdp_device::get_tile_info)), TILEMAP_SCAN_ROWS, 16, 8, 32, 64);

	save_item(NAME(m_vram_addr));
	save_item(NAME(m_prev_writes));
}

void janshi_vdp_device::device_reset() {}

device_memory_interface::space_config_vector janshi_vdp_device::memory_space_config() const
{
	return space_config_vector {
		std::make_pair(0, &m_space_config)
	};
}

void janshi_vdp_device::back_vram_w(offs_t offset, uint8_t data)
{
	m_back_vram[offset] = data;
	m_tilemap->mark_tile_dirty(offset >> 2);
}

void janshi_vdp_device::write(offs_t offset, uint8_t data)
{
	switch (offset)
	{
		case 0:
			m_vram_addr = (data << 0)  | (m_vram_addr & 0xffff00);
			LOGMASKED(LOG_VRAM, "\n prev writes was %04x\n\naddress set to %04x -\n", m_prev_writes, m_vram_addr);
			m_prev_writes = 0;
			break;

		case 1:
			m_vram_addr = (data << 8)  | (m_vram_addr & 0xff00ff);
			LOGMASKED(LOG_VRAM, "\naddress set to %04x\n", m_vram_addr);
			break;

		case 2:
			m_vram_addr = (data << 16) | (m_vram_addr & 0x00ffff);
			LOGMASKED(LOG_VRAM, "\naddress set to %04x\n", m_vram_addr);
			break;

		case 3:
		{
			LOGMASKED(LOG_VRAM, "%02x ", data);
			m_prev_writes++;
			m_vram_addr++;

			space(0).write_byte(m_vram_addr, data);
			break;
		}
	}
}

TILE_GET_INFO_MEMBER(janshi_vdp_device::get_tile_info)
{
	/* FIXME: color is a bit of a mystery */
	tile_index <<= 2;
	int const tile = m_back_vram[tile_index + 1] << 8 | m_back_vram[tile_index + 0];
	int const attr = m_back_vram[tile_index + 2] ^ 0xf0;
	int const col = (attr >> 4) | 0x10;

	tileinfo.set(0, tile, col, 0);
}

void janshi_vdp_device::draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	//popmessage("%02x", m_crtc_regs[0x0a]);
	int const col_bank = BIT(m_crtc_regs[0x0a], 6);

	for (int i = (0x1000 / 4) - 4; i >= 0; i--)
	{
		/*
		"vram1" (video map 0xfc2000)

		tttt tttt | 00tt tttt | cccc c000 | xxxx xxxx |

		"vram2" (video map 0xfc3800)

		yyyy yyyy | ???? ???? |


		widths come from "widthflags" (0xfc3780)
		"unk1" (0xfc3700) and "unk2" (0xfc37c0) are a mystery
		*/

		int spr_offs = (m_vram1[(i * 4) + 0] | (m_vram1[(i * 4) + 1] << 8)) & 0xffff;
		int col = (m_vram1[(i * 4) + 2] & 0xf8) >> 3;
		int x =   m_vram1[(i * 4) + 3] * 2;

		//int unk2 = m_vram2[(i * 2) + 1];
		int y = (m_vram2[(i * 2) + 0]);
		y = 0x100 - y;

		col |= col_bank << 5;

		//int width = 0, height = 0;

		int width = 2;
		int height = 2;

		// this bit determines the sprite width, one bit is used in each word, each bit is used for a range of sprites
		int const bit = m_widthflags[(i / 0x20) * 2 + 1];

		if (bit)
		{
			//col = machine().rand();
			width = 2;
		}
		else
		{
			width = 1;
			height = 2;
		}

		// hacks!
		if (m_janshi_hack) // janshi
		{
			if (spr_offs < 0x400)
			{
				height = 4;
			}
			else if (spr_offs < 0x580)
			{
			//  height = 2;
			}
			else if (spr_offs < 0x880)
			{
				height = 4;
			}
			else if (spr_offs < 0x1000)
			{
			//  height = 2;
			}
			else if (spr_offs < 0x1080)
			{
			//  height = 2;
			}
			else if (spr_offs < 0x1700)
			{
				height = 4;
			}
			else if (spr_offs < 0x1730)
			{
			//  height = 2;
			}
			else if (spr_offs < 0x1930)
			{
				height = 4;
			}
			else if (spr_offs < 0x19c0)
			{
				height = 1;
			}
			else
			{
				height = 4;
			}
		}

		if (height == 1)
			y += 16;

		// hmm...
		if (height == 2)
			y += 16;

		int count = 0;

		for (int yy = 0; yy < height; yy++)
		{
			for (int xx = 0; xx < width; xx++)
			{
				gfx(0)->transpen(bitmap, cliprect,
						spr_offs + count, col,
						0, 0,
						(x + xx * 16) - 7, (y + yy * 8) - 33,
						0);
				count++;
			}
		}
	}
}

uint32_t janshi_vdp_device::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
#if 0
	if (machine().input().code_pressed_once(KEYCODE_W))
	{
		printf("-------------------------------\n");
		int count2 = 0;
		for (int i = 0x00; i < 0x40; i += 2)
		{
			printf("%02x, ", m_widthflags[i+1]);

			count2++;

			if (count2 == 0x10)
			{
				printf("\n");
				count2 = 0;
			}
		}
	}
#endif

	bitmap.fill(m_palette->black_pen(), cliprect);

	m_tilemap->draw(screen, bitmap, cliprect, 0, 0);

	draw_sprites(bitmap, cliprect);

	return 0;
}

void pinkiri8_state::prg_map(address_map &map)
{
	map(0x00000, 0x0bfff).rom();
	map(0x0c000, 0x0dfff).ram();
	map(0x0e000, 0x0ffff).rom();
	map(0x10000, 0x1ffff).rom();
}

void pinkiri8_state::output_regs_w(uint8_t data)
{
	if (BIT(data, 6))
		m_maincpu->set_input_line(INPUT_LINE_NMI, CLEAR_LINE);
	//BIT(data, 7) is probably NMI mask
}

void pinkiri8_state::io_matrix_w(uint8_t data)
{
	m_io_matrix = data;
}

template <unsigned Player>
uint8_t pinkiri8_state::io_matrix_r()
{
	uint8_t ret = 0xff;
	for (int i = 0; i < 5; i++)
	{
		if (BIT(m_io_matrix, i))
			ret &= m_io_pl[Player][i]->read();
	}

	return ret;
}

void pinkiri8_state::pinkiri8_io(address_map &map)
{
	map.global_mask(0xff);
	map(0x00, 0x3f).ram(); //Z180 internal I/O
	map(0x60, 0x60).nopw();
	map(0x80, 0x83).w(m_vdp, FUNC(janshi_vdp_device::write));

	map(0xa0, 0xa0).rw("oki", FUNC(okim6295_device::read), FUNC(okim6295_device::write)); //correct?
	map(0xb0, 0xb0).w(FUNC(pinkiri8_state::io_matrix_w)); //mux
	map(0xb0, 0xb0).r(FUNC(pinkiri8_state::io_matrix_r<1>)); // mux inputs
	map(0xb1, 0xb1).r(FUNC(pinkiri8_state::io_matrix_r<0>)); // mux inputs
	map(0xb2, 0xb2).portr("SYSTEM");
	map(0xf8, 0xf8).portr("DSW1");
	map(0xf9, 0xf9).portr("DSW2");
	map(0xfa, 0xfa).portr("DSW3");
	map(0xfb, 0xfb).portr("DSW4");

	/* Wing custom sound chip, same as Lucky Girl Z180 */
	map(0xc3, 0xc3).nopw();
	map(0xc7, 0xc7).nopw();
	map(0xcb, 0xcb).nopw();
	map(0xcf, 0xcf).nopw();

	map(0xd3, 0xd3).nopw();
	map(0xd7, 0xd7).nopw();
	map(0xdb, 0xdb).nopw();
	map(0xdf, 0xdf).nopw();

	map(0xe3, 0xe3).nopw();
	map(0xe7, 0xe7).nopw();
	map(0xeb, 0xeb).nopw();
	map(0xef, 0xef).nopw();

	map(0xf3, 0xf3).nopw();
	map(0xf7, 0xf7).nopw();
}

void pinkiri8_state::ronjan_io(address_map &map)
{
	pinkiri8_io(map);
	map(0x90, 0x90).rw(FUNC(pinkiri8_state::ronjan_prot_r), FUNC(pinkiri8_state::ronjan_prot_w));
	map(0x9f, 0x9f).r(FUNC(pinkiri8_state::ronjan_patched_prot_r));
}

static INPUT_PORTS_START( base_inputs )
	PORT_START("SYSTEM")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_NAME("Reset SW")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE2 ) PORT_NAME("Books SW")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_GAMBLE_KEYOUT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1) //ron jan needs this
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_GAMBLE_KEYIN )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PL1_1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_A )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_E )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_I )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_M )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_KAN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START("PL1_2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_B )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_F )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_J )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_N )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_REACH )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_BET )

	PORT_START("PL1_3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_C )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_G )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_K )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_CHI )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_RON )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PL1_4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_D )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_H )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_L )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_PON )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PL1_5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_LAST_CHANCE )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_SCORE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_DOUBLE_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_BIG )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_SMALL )

	PORT_START("PL2_1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_A ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_E ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_I ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_M ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_KAN ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START("PL2_2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_B ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_F ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_J ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_N ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_REACH ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_BET ) PORT_PLAYER(2)

	PORT_START("PL2_3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_C ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_G ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_K ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_CHI ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_RON ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PL2_4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_D ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_H ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_L ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_PON ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PL2_5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_LAST_CHANCE ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_SCORE ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_DOUBLE_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_BIG ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_SMALL ) PORT_PLAYER(2)

	PORT_START("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "DSW1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("DSW2")
	PORT_DIPNAME( 0x01, 0x01, "DSW2" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("DSW3")
	PORT_DIPNAME( 0x01, 0x01, "DSW3" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("DSW4")
	PORT_DIPNAME( 0x01, 0x01, "DSW4" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static INPUT_PORTS_START( janshi )
	PORT_INCLUDE( base_inputs )

	PORT_MODIFY("DSW1")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) ) PORT_DIPLOCATION("SW1:1,2,3")
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_10C ))
	PORT_DIPNAME( 0x08, 0x08, "Round Up Bonus" ) PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x08, "500" )
	PORT_DIPSETTING(    0x00, "1000" )
	PORT_DIPNAME( 0x30, 0x00, "Base Score" ) PORT_DIPLOCATION("SW1:5,6")
	PORT_DIPSETTING(    0x00, "5000 / 8000" )
	PORT_DIPSETTING(    0x10, "4000 / 8000" )
	PORT_DIPSETTING(    0x20, "3000 / 8000" )
	PORT_DIPSETTING(    0x30, "2000 / 8000" )
	PORT_DIPNAME( 0xc0, 0x80, "Win Rate" ) PORT_DIPLOCATION("SW1:7,8")
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Hard ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Very_Easy ) )

	PORT_MODIFY("DSW2")
	PORT_DIPNAME( 0x03, 0x01, "Play Time" ) PORT_DIPLOCATION("SW2:1,2")
	PORT_DIPSETTING(    0x00, "12 Seconds" )
	PORT_DIPSETTING(    0x01, "10 Seconds" )
	PORT_DIPSETTING(    0x02, "8 Seconds" )
	PORT_DIPSETTING(    0x03, "6 Seconds" )
	PORT_DIPNAME( 0x04, 0x04, "Yakuman Bonus" ) PORT_DIPLOCATION("SW2:3")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Free_Play ) ) PORT_DIPLOCATION("SW2:4")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, "BGM" ) PORT_DIPLOCATION("SW2:5")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, "Voice" ) PORT_DIPLOCATION("SW2:6")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "Nudity" ) PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) ) PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_MODIFY("DSW3")
	PORT_DIPUNUSED_DIPLOC( 0x01, 0x01, "SW3:1" )
	PORT_DIPUNUSED_DIPLOC( 0x02, 0x02, "SW3:2" )
	PORT_DIPUNUSED_DIPLOC( 0x04, 0x04, "SW3:3" )
	PORT_DIPUNUSED_DIPLOC( 0x08, 0x08, "SW3:4" )
	PORT_DIPUNUSED_DIPLOC( 0x10, 0x10, "SW3:5" )
	PORT_DIPUNUSED_DIPLOC( 0x20, 0x20, "SW3:6" )
	PORT_DIPUNUSED_DIPLOC( 0x40, 0x40, "SW3:7" )
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW3:8" )

	PORT_MODIFY("DSW4")
	PORT_DIPUNUSED_DIPLOC( 0x01, 0x01, "SW4:1" )
	PORT_DIPUNUSED_DIPLOC( 0x02, 0x02, "SW4:2" )
	PORT_DIPUNUSED_DIPLOC( 0x04, 0x04, "SW4:3" )
	PORT_DIPUNUSED_DIPLOC( 0x08, 0x08, "SW4:4" )
	PORT_DIPUNUSED_DIPLOC( 0x10, 0x10, "SW4:5" )
	PORT_DIPUNUSED_DIPLOC( 0x20, 0x20, "SW4:6" )
	PORT_DIPUNUSED_DIPLOC( 0x40, 0x40, "SW4:7" )
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW4:8" )
INPUT_PORTS_END

static INPUT_PORTS_START( ronjan )
	PORT_INCLUDE( base_inputs )

	PORT_MODIFY("DSW1")
	PORT_DIPNAME( 0x03, 0x00, "Play Time Limit" ) PORT_DIPLOCATION("SW1:1,2")
	PORT_DIPSETTING(    0x00, "16 Seconds" )
	PORT_DIPSETTING(    0x01, "13 Seconds" )
	PORT_DIPSETTING(    0x02, "10 Seconds" )
	PORT_DIPSETTING(    0x03, "7 Seconds" )
	PORT_DIPNAME( 0x04, 0x04, "Coin Payment" ) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, "Auto" )
	//FIXME: Bet Min can't be higher than Bet Max, needs a conditional dip here (or maybe, unified dips)
	PORT_DIPNAME( 0x38, 0x38, "Bet Min" ) PORT_DIPLOCATION("SW1:4,5,6")
	PORT_DIPSETTING(    0x38, "1" )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x28, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x18, "5" )
	PORT_DIPSETTING(    0x10, "6" )
	PORT_DIPSETTING(    0x08, "8" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0xc0, 0x00, "Bet Max" ) PORT_DIPLOCATION("SW1:7,8")
	PORT_DIPSETTING(    0xc0, "5" )
	PORT_DIPSETTING(    0x80, "10" )
	PORT_DIPSETTING(    0x40, "15" )
	PORT_DIPSETTING(    0x00, "20" )

	PORT_MODIFY("DSW2")
	PORT_DIPNAME( 0x0f, 0x00, "Rate of Win" ) PORT_DIPLOCATION("SW2:1,2,3,4")
	PORT_DIPSETTING(    0x00, "98%" )
	PORT_DIPSETTING(    0x01, "95%" )
	PORT_DIPSETTING(    0x02, "92%" )
	PORT_DIPSETTING(    0x03, "89%" )
	PORT_DIPSETTING(    0x04, "86%" )
	PORT_DIPSETTING(    0x05, "83%" )
	PORT_DIPSETTING(    0x06, "80%" )
	PORT_DIPSETTING(    0x07, "77%" )
	PORT_DIPSETTING(    0x08, "74%" )
	PORT_DIPSETTING(    0x09, "71%" )
	PORT_DIPSETTING(    0x0a, "68%" )
	PORT_DIPSETTING(    0x0b, "65%" )
	PORT_DIPSETTING(    0x0c, "62%" )
	PORT_DIPSETTING(    0x0d, "59%" )
	PORT_DIPSETTING(    0x0e, "56%" )
	PORT_DIPSETTING(    0x0f, "53%" )
	PORT_DIPNAME( 0x10, 0x10, "Limit Display" ) PORT_DIPLOCATION("SW2:5")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0xe0, 0xe0, "Credit Limit" )  PORT_DIPLOCATION("SW2:6,7,8")
	PORT_DIPSETTING(    0x00, "50000" )
	PORT_DIPSETTING(    0x20, "40000" )
	PORT_DIPSETTING(    0x40, "30000" )
	PORT_DIPSETTING(    0x60, "20000" )
	PORT_DIPSETTING(    0x80, "15000" )
	PORT_DIPSETTING(    0xa0, "10000" )
	PORT_DIPSETTING(    0xc0, "5000" )
	PORT_DIPSETTING(    0xe0, "No Limit" )

	PORT_MODIFY("DSW3")
	PORT_DIPNAME( 0x07, 0x07, "Key In Coinage" ) PORT_DIPLOCATION("SW3:1,2,3")
	PORT_DIPSETTING(    0x00, "1 Coin/500 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/200 Credits" )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_100C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_50C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_25C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_20C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_10C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x78, 0x40, DEF_STR( Coinage ) ) PORT_DIPLOCATION("SW3:4,5,6,7")
	PORT_DIPSETTING(    0x78, DEF_STR( 10C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x68, DEF_STR( 5C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x58, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x48, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_10C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_20C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_25C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_50C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_100C ) )
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW3:8" )

	PORT_MODIFY("DSW4")
	PORT_DIPNAME( 0x01, 0x01, "Odds Type" ) PORT_DIPLOCATION("SW4:1")
	PORT_DIPSETTING(    0x01, "A" )
	PORT_DIPSETTING(    0x00, "B" )
	PORT_DIPNAME( 0x02, 0x02, "Special Bonus Odds" ) PORT_DIPLOCATION("SW4:2")
	PORT_DIPSETTING(    0x02, "A" )
	PORT_DIPSETTING(    0x00, "B" )
	PORT_DIPNAME( 0x04, 0x00, "Kind Mark of Back" ) PORT_DIPLOCATION("SW4:3")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "Nudity" ) PORT_DIPLOCATION("SW4:4")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, "BGM" ) PORT_DIPLOCATION("SW4:5")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, "Voice" ) PORT_DIPLOCATION("SW4:6")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "Double Up Game" ) PORT_DIPLOCATION("SW4:7")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, "Double Up Limit" ) PORT_DIPLOCATION("SW4:8")
	PORT_DIPSETTING(    0x80, "6" )
	PORT_DIPSETTING(    0x00, "No Limit" )
INPUT_PORTS_END

static INPUT_PORTS_START( pinkiri8 )
	PORT_INCLUDE( base_inputs )

	/* standard mahjong panel converted to a hanafuda one */
	PORT_MODIFY("PL1_1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_HANAFUDA_A )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_HANAFUDA_E )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_HANAFUDA_YES )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )

	PORT_MODIFY("PL1_2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_HANAFUDA_B )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_HANAFUDA_F )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_HANAFUDA_NO )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_BET )

	PORT_MODIFY("PL1_3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_HANAFUDA_C )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_HANAFUDA_G )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_MODIFY("PL1_4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_HANAFUDA_D )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_HANAFUDA_H )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_MODIFY("PL1_5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_MODIFY("PL2_1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_HANAFUDA_A ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_HANAFUDA_E ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_HANAFUDA_YES ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )

	PORT_MODIFY("PL2_2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_HANAFUDA_B ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_HANAFUDA_F ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_HANAFUDA_NO ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_BET ) PORT_PLAYER(2)

	PORT_MODIFY("PL2_3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_HANAFUDA_C ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_HANAFUDA_G ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_MODIFY("PL2_4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_HANAFUDA_D ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_HANAFUDA_H ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_MODIFY("PL2_5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_MODIFY("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "Game Style" ) PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x01, "Win / Bet" )
	PORT_DIPSETTING(    0x00, "Out / In" )
	PORT_DIPNAME( 0x02, 0x02, "Premium Hand" ) PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Payment" ) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, "Auto" )
	//FIXME: Bet Min can't be higher than Bet Max, needs a conditional dip here (or maybe, unified dips)
	PORT_DIPNAME( 0x38, 0x38, "Bet Min" ) PORT_DIPLOCATION("SW1:4,5,6")
	PORT_DIPSETTING(    0x38, "1" )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x28, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x18, "5" )
	PORT_DIPSETTING(    0x10, "6" )
	PORT_DIPSETTING(    0x08, "8" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0xc0, 0x00, "Bet Max" ) PORT_DIPLOCATION("SW1:7,8")
	PORT_DIPSETTING(    0xc0, "5" )
	PORT_DIPSETTING(    0x80, "10" )
	PORT_DIPSETTING(    0x40, "15" )
	PORT_DIPSETTING(    0x00, "20" )

	PORT_MODIFY("DSW2")
	PORT_DIPNAME( 0x0f, 0x00, "Rate of Win" ) PORT_DIPLOCATION("SW2:1,2,3,4")
	PORT_DIPSETTING(    0x00, "90%" )
	PORT_DIPSETTING(    0x01, "86%" )
	PORT_DIPSETTING(    0x02, "82%" )
	PORT_DIPSETTING(    0x03, "78%" )
	PORT_DIPSETTING(    0x04, "74%" )
	PORT_DIPSETTING(    0x05, "70%" )
	PORT_DIPSETTING(    0x06, "66%" )
	PORT_DIPSETTING(    0x07, "62%" )
	PORT_DIPSETTING(    0x08, "58%" )
	PORT_DIPSETTING(    0x09, "54%" )
	PORT_DIPSETTING(    0x0a, "50%" )
	PORT_DIPSETTING(    0x0b, "46%" )
	PORT_DIPSETTING(    0x0c, "42%" )
	PORT_DIPSETTING(    0x0d, "38%" )
	PORT_DIPSETTING(    0x0e, "34%" )
	PORT_DIPSETTING(    0x0f, "30%" )
	PORT_DIPNAME( 0x10, 0x10, "Odds Type" ) PORT_DIPLOCATION("SW2:5")
	PORT_DIPSETTING(    0x10, "A" )
	PORT_DIPSETTING(    0x00, "B" )
	PORT_DIPNAME( 0x20, 0x20, "BGM" ) PORT_DIPLOCATION("SW2:6")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, "Oya (Owner)" ) PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(    0x40, "CPU Only" )
	PORT_DIPSETTING(    0x00, "Winner" )
	PORT_DIPNAME( 0x80, 0x00, "Koi Time Limit" ) PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(    0x80, "15 Seconds" )
	PORT_DIPSETTING(    0x00, "30 Seconds" )

	PORT_MODIFY("DSW3")
	PORT_DIPNAME( 0x07, 0x07, "Key In Coinage" ) PORT_DIPLOCATION("SW3:1,2,3")
	PORT_DIPSETTING(    0x00, "1 Coin/500 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/200 Credits" )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_100C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_50C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_25C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_20C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_10C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x78, 0x40, DEF_STR( Coinage ) ) PORT_DIPLOCATION("SW3:4,5,6,7")
	PORT_DIPSETTING(    0x78, DEF_STR( 10C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x68, DEF_STR( 5C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x58, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x48, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_10C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_20C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_25C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_50C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_100C ) )
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW3:8" )

	PORT_MODIFY("DSW4")
	PORT_DIPNAME( 0x01, 0x01, "Pinkiri Bonus" ) PORT_DIPLOCATION("SW4:1")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, "Same Month Bonus" ) PORT_DIPLOCATION("SW4:2")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x00, "Play Time Limit" ) PORT_DIPLOCATION("SW4:3")
	PORT_DIPSETTING(    0x04, "7 Seconds" )
	PORT_DIPSETTING(    0x00, "12 Seconds" )
	PORT_DIPNAME( 0x18, 0x00, "Credit Clear" ) PORT_DIPLOCATION("SW4:4,5")
	PORT_DIPSETTING(    0x00, "300 Seconds" )
	PORT_DIPSETTING(    0x08, "180 Seconds" )
	PORT_DIPSETTING(    0x10, "120 Seconds" )
	PORT_DIPSETTING(    0x18, "60 Seconds" )
	PORT_DIPNAME( 0x20, 0x20, "Panel Type" ) PORT_DIPLOCATION("SW4:6") // no real difference?
	PORT_DIPSETTING(    0x20, "Mahjong" )
	PORT_DIPSETTING(    0x00, "Hanafuda" )
	PORT_DIPNAME( 0x40, 0x40, "Flip Flop Button" ) PORT_DIPLOCATION("SW4:7")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW4:8" )
INPUT_PORTS_END

void pinkiri8_state::machine_start()
{
	save_item(NAME(m_io_matrix));

	save_item(NAME(m_prot_read_index));
	save_item(NAME(m_prot_char));
	save_item(NAME(m_prot_index));
}

void pinkiri8_state::pinkiri8(machine_config &config)
{
	HD647180X(config, m_maincpu, XTAL(32'000'000)/2);
	m_maincpu->set_addrmap(AS_PROGRAM, &pinkiri8_state::prg_map);
	m_maincpu->set_addrmap(AS_IO, &pinkiri8_state::pinkiri8_io);
	m_maincpu->set_vblank_int("screen", FUNC(pinkiri8_state::nmi_line_assert));
	m_maincpu->out_pa_callback().set(FUNC(pinkiri8_state::output_regs_w));

	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
	screen.set_refresh_hz(60);
	screen.set_vblank_time(ATTOSECONDS_IN_USEC(0));
	screen.set_size(64*8, 64*8);
	screen.set_visarea(0*8, 62*8-1, 0*8, 32*8-1);
	screen.set_screen_update(m_vdp, FUNC(janshi_vdp_device::screen_update));
	screen.set_palette("janshivdp:palette");

	JANSHIVDP(config, m_vdp, 0);

	/* sound hardware */
	SPEAKER(config, "mono").front_center();

	OKIM6295(config, "oki", 1056000, okim6295_device::PIN7_HIGH).add_route(ALL_OUTPUTS, "mono", 0.5); // clock frequency & pin 7 not verified
}

void pinkiri8_state::ronjan(machine_config &config)
{
	pinkiri8(config);

	m_maincpu->set_addrmap(AS_IO, &pinkiri8_state::ronjan_io);
	m_maincpu->in_pg_callback().set(FUNC(pinkiri8_state::ronjan_prot_status_r));
}

void pinkiri8_state::janshi(machine_config &config)
{
	pinkiri8(config);

	m_vdp->set_janshi_hack(true);
}

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( pinkiri8 )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "pinkiri8-ver.1.02.l1",   0x0000, 0x20000, CRC(f2df5b12) SHA1(e374e184a6a1e932550516011ec09a5accec9b03) )
	ROM_LOAD( "bios.rom", 0x0000, 0x4000, CRC(399df1ee) SHA1(8251f3aa7da4c7899c8e739c10b61260f4471311) ) //overlapped internal ROM

	ROM_REGION( 0x20000*5, "janshivdp", 0 )
	ROM_LOAD( "pinkiri8-chr-01.a1",  0x00000, 0x20000, CRC(8ec73662) SHA1(9098348e519ce753dd7f38f0d855181bfc65aa42) )
	ROM_LOAD( "pinkiri8-chr-02.bc1", 0x20000, 0x20000, CRC(8dc20a65) SHA1(4062510fe06e8844a732754b7915a3b67ba2a3c5) )
	ROM_LOAD( "pinkiri8-chr-03.d1",  0x40000, 0x20000, CRC(bd5f269a) SHA1(7dfd039227551f0f0ed4afaafc76ca64a39a9b83) )
	ROM_LOAD( "pinkiri8-chr-04.ef1", 0x60000, 0x20000, CRC(4d0e5005) SHA1(4b90119c359c4de576131fd0e28d2fe1482ce74f) )
	ROM_LOAD( "pinkiri8-chr-05.h1",  0x80000, 0x20000, CRC(036ca165) SHA1(c4a2d6e394bbabcae1413d8a2916a19c90687edf) )

	ROM_REGION( 0x40000, "oki", ROMREGION_ERASE00 )
ROM_END


ROM_START( janshi )
	ROM_REGION( 0x24000, "maincpu", 0 )
	ROM_LOAD( "11.1l",    0x00000, 0x20000, CRC(a7692ddf) SHA1(5e7f43d8337583977baf22a28bbcd9b2182c0cde) )
	ROM_LOAD( "=3= 9009 1992.1 new jansh.bin", 0x0000, 0x4000, CRC(63cd3f12) SHA1(aebac739bffaf043e6acffa978e935f73ee1385f) ) //overlapped internal ROM

	ROM_REGION( 0x140000, "janshivdp", 0 )
	ROM_LOAD( "1.1a", 0x000000, 0x40000, CRC(92b140a5) SHA1(f3b38563f74650604ed0faaf84460e0b04b386b7) )
	ROM_LOAD( "2.1b", 0x040000, 0x40000, CRC(6de7e086) SHA1(e87426264f0181c17383ffe0f7ec7ff5fce3d809) )
	ROM_LOAD( "3.1d", 0x080000, 0x40000, CRC(4e94d8f2) SHA1(a25f542943d74915fc82910baafb9ff9db1ffd70) )
	ROM_LOAD( "4.1f", 0x0c0000, 0x40000, CRC(a5f6e3ef) SHA1(f1f3d28b27eea682aa71855a311fb3abdf9af2cd) )
	ROM_LOAD( "5.1h", 0x100000, 0x40000, CRC(ff2cc769) SHA1(ba4cf2923cf3d4d815a9327595f8e1801c3c8a2b) )

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "6.1k", 0x00000, 0x40000, CRC(8197034d) SHA1(b501dc7a27b1faad1361c309afd726da14b8b5f5) )
ROM_END

ROM_START( ronjans )
	ROM_REGION( 0x24000, "maincpu", 0 )
	ROM_LOAD( "ver201.bin",    0x00000, 0x20000, CRC(caa98c79) SHA1(e18f52fc910e3a77142ad2a3167805cfd664f0f4) )
	ROM_LOAD( "9009 1996.08 ron jan.bin", 0x00000, 0x4000, CRC(4eb74322) SHA1(84f864c0da3fb69948f6eb7ffecf0e722a882efc) ) //overlapped internal ROM

	ROM_REGION( 0x140000, "janshivdp", 0 )
	ROM_LOAD( "eagle.1", 0x000000, 0x40000, CRC(11cef2c4) SHA1(fcd46bfa123cd91053f8d49892778e02a275ffdd) )
	ROM_LOAD( "eagle.2", 0x040000, 0x40000, CRC(177c444c) SHA1(5af0f6040ba121c90b3480ce636885cce535d3ea) )
	ROM_LOAD( "eagle.3", 0x080000, 0x40000, CRC(5b15b99f) SHA1(b99e2fa4cde7c8661d1a81ce5045f5df4f1de9f2) )
	ROM_LOAD( "eagle.4", 0x0c0000, 0x40000, CRC(d6797340) SHA1(0394ba570f2008f5a16e7c0a4dc67b1182be8899) )
	ROM_LOAD( "eagle.5", 0x100000, 0x40000, CRC(1aa42eaf) SHA1(edae2d1b58429e09ecfcaa5bcf4a9bfd5fb7cbea) )

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "eagle.6", 0x00000, 0x40000, CRC(8197034d) SHA1(b501dc7a27b1faad1361c309afd726da14b8b5f5) )
ROM_END

ROM_START( ronjansa ) // the Z180 internal ROM wasn't extracted from this PCB. It's not compatible with already dumped ones.
	ROM_REGION( 0x24000, "maincpu", 0 )
	ROM_LOAD( "eagle_18.i1",              0x00000, 0x20000, CRC(b5cc6d84) SHA1(e76ec529a7cd788a9ca0119d2f2dc00b29181289) )
	ROM_LOAD( "9009 1992.04 ron jan.bin", 0x00000, 0x04000, NO_DUMP ) //overlapped internal ROM

	ROM_REGION( 0x140000, "janshivdp", 0 )
	ROM_LOAD( "eagle_1.a1", 0x000000, 0x40000, CRC(11cef2c4) SHA1(fcd46bfa123cd91053f8d49892778e02a275ffdd) )
	ROM_LOAD( "eagle_2.b1", 0x040000, 0x40000, CRC(177c444c) SHA1(5af0f6040ba121c90b3480ce636885cce535d3ea) )
	ROM_LOAD( "eagle_3.d1", 0x080000, 0x40000, CRC(5b15b99f) SHA1(b99e2fa4cde7c8661d1a81ce5045f5df4f1de9f2) )
	ROM_LOAD( "eagle_4.e1", 0x0c0000, 0x40000, CRC(d6797340) SHA1(0394ba570f2008f5a16e7c0a4dc67b1182be8899) )
	ROM_LOAD( "eagle_5.h1", 0x100000, 0x40000, CRC(1aa42eaf) SHA1(edae2d1b58429e09ecfcaa5bcf4a9bfd5fb7cbea) )

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "eagle_6.j1", 0x00000, 0x40000, CRC(8197034d) SHA1(b501dc7a27b1faad1361c309afd726da14b8b5f5) )
ROM_END

ROM_START( ronjansb ) // the Z180 internal ROM wasn't extracted from this PCB. It's not compatible with already dumped ones. Should be same as ronjansa
	ROM_REGION( 0x24000, "maincpu", 0 )
	ROM_LOAD( "eagle_19.i1",              0x00000, 0x20000, CRC(348fa965) SHA1(082395c51478c1cc053425d30fc94871fdc244ea) )
	ROM_LOAD( "9009 1992.09 ron jan.bin", 0x00000, 0x04000, NO_DUMP ) //overlapped internal ROM

	ROM_REGION( 0x140000, "janshivdp", 0 )
	ROM_LOAD( "eagle_1.a1", 0x000000, 0x40000, CRC(11cef2c4) SHA1(fcd46bfa123cd91053f8d49892778e02a275ffdd) )
	ROM_LOAD( "eagle_2.b1", 0x040000, 0x40000, CRC(177c444c) SHA1(5af0f6040ba121c90b3480ce636885cce535d3ea) )
	ROM_LOAD( "eagle_3.d1", 0x080000, 0x40000, CRC(5b15b99f) SHA1(b99e2fa4cde7c8661d1a81ce5045f5df4f1de9f2) )
	ROM_LOAD( "eagle_4.e1", 0x0c0000, 0x40000, CRC(d6797340) SHA1(0394ba570f2008f5a16e7c0a4dc67b1182be8899) )
	ROM_LOAD( "eagle_5.h1", 0x100000, 0x40000, CRC(1aa42eaf) SHA1(edae2d1b58429e09ecfcaa5bcf4a9bfd5fb7cbea) )

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "eagle_6.j1", 0x00000, 0x40000, CRC(8197034d) SHA1(b501dc7a27b1faad1361c309afd726da14b8b5f5) )
ROM_END

ROM_START( ronjan ) // the Z180 internal ROM wasn't extracted from this PCB. Using the one from ronjans for the time being, which might be the same but should be checked.
	ROM_REGION( 0x24000, "maincpu", 0 )
	ROM_LOAD( "9.l1",    0x00000, 0x20000, CRC(1bc4468e) SHA1(5b317c922d9a6f533958526e676f95af0ee6a19f) )
	ROM_LOAD( "9009 1991.11 ron jan.bin", 0x00000, 0x4000, BAD_DUMP CRC(4eb74322) SHA1(84f864c0da3fb69948f6eb7ffecf0e722a882efc) ) //overlapped internal ROM

	ROM_REGION( 0x140000, "janshivdp", 0 )
	ROM_LOAD( "1.a1", 0x000000, 0x20000, CRC(8242a791) SHA1(bb753e81293685499513e83b7a103396b3a32ad8) )
	ROM_LOAD( "2.c1", 0x040000, 0x20000, CRC(4b25c09a) SHA1(edbe1907c300f12bf65c81b2d9e034d6f5545bd0) )
	ROM_LOAD( "3.d1", 0x080000, 0x20000, CRC(7b956af6) SHA1(4a661d5cc5b06658804c8d377d5a266f5bd9ce85) )
	ROM_LOAD( "4.f1", 0x0c0000, 0x20000, CRC(4bebed0b) SHA1(f6e95b3aad1905a397b594db43c65902330945f4) )
	ROM_LOAD( "5.h1", 0x100000, 0x20000, CRC(ec1d36bf) SHA1(5db8cfeea40a85ba62730976b15e8ed00e541dd2) )

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "6.j1", 0x00000, 0x20000, CRC(d0b53513) SHA1(e94402f494adae741989c98a8c9587f464f144d2) )
ROM_END

ROM_START( ronjana ) // the Z180 internal ROM wasn't extracted from this PCB. Using the one from ronjans for the time being, which might be the same but should be checked.
	ROM_REGION( 0x24000, "maincpu", 0 )
	ROM_LOAD( "eagle_16.i1",              0x00000, 0x20000, CRC(9b7bf916) SHA1(d8a732bb53926e8127bc3638c8719f3c43c7881d) )
	ROM_LOAD( "9009 1991.11 ron jan.bin", 0x00000, 0x04000, BAD_DUMP CRC(4eb74322) SHA1(84f864c0da3fb69948f6eb7ffecf0e722a882efc) ) //overlapped internal ROM

	ROM_REGION( 0x140000, "janshivdp", 0 )
	ROM_LOAD( "eagle_1.a1", 0x000000, 0x40000, CRC(11cef2c4) SHA1(fcd46bfa123cd91053f8d49892778e02a275ffdd) )
	ROM_LOAD( "eagle_2.b1", 0x040000, 0x40000, CRC(177c444c) SHA1(5af0f6040ba121c90b3480ce636885cce535d3ea) )
	ROM_LOAD( "eagle_3.d1", 0x080000, 0x40000, CRC(5b15b99f) SHA1(b99e2fa4cde7c8661d1a81ce5045f5df4f1de9f2) )
	ROM_LOAD( "eagle_4.e1", 0x0c0000, 0x40000, CRC(d6797340) SHA1(0394ba570f2008f5a16e7c0a4dc67b1182be8899) )
	ROM_LOAD( "eagle_5.h1", 0x100000, 0x40000, CRC(1aa42eaf) SHA1(edae2d1b58429e09ecfcaa5bcf4a9bfd5fb7cbea) )

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "eagle_6.j1", 0x00000, 0x40000, CRC(8197034d) SHA1(b501dc7a27b1faad1361c309afd726da14b8b5f5) )
ROM_END

ROM_START( janmu ) // 5019 8009-A PCB
	ROM_REGION( 0x24000, "maincpu", 0 )
	ROM_LOAD( "9.1l",                       0x00000, 0x20000, CRC(970e7e78) SHA1(1d7372323e5b57f2dfdfa166e389023f00a3067a) )
	ROM_LOAD( "9009 1990.12 l-13 janmu.1r", 0x00000, 0x04000, NO_DUMP ) //overlapped internal ROM

	ROM_REGION( 0xa0000, "janshivdp", 0 )
	ROM_LOAD( "eagle_1.1a", 0x000000, 0x20000, CRC(47ff0a55) SHA1(404ce1de03e1850250ff0fc3c80365e6166403ee) )
	ROM_LOAD( "eagle_2.1b", 0x020000, 0x20000, CRC(bbf08fbe) SHA1(a0cd603d71e73a9eeae82473a539e8056619f72a) )
	ROM_LOAD( "eagle_3.1d", 0x040000, 0x20000, CRC(6f74ea6e) SHA1(16788151c98dc660a3d8bb70f298e4f20c66537e) )
	ROM_LOAD( "eagle_4.1e", 0x060000, 0x20000, CRC(9e4c34db) SHA1(576622d156be5296e994b154fa2263b5647c2841) )
	ROM_LOAD( "eagle_5.1h", 0x080000, 0x20000, CRC(0dbb1801) SHA1(c07c7525102184c5b42cfb6b79907e48bf0eb459) )

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "7.1j", 0x00000, 0x20000, CRC(0916ba15) SHA1(db1fe3e19ecf2da565cc397679164d9d614a4ed8) )
	ROM_LOAD( "8.2j", 0x20000, 0x20000, CRC(e8c26032) SHA1(e0b6e86a435bed668ac6503f28a23d63f89a4b31) )
ROM_END

uint8_t pinkiri8_state::ronjan_prot_r()
{
	static const char wing_str[6] = { 'W', 'I', 'N', 'G', '8', '9' };

	uint8_t prot_read_index = m_prot_read_index + 1;
	if (!machine().side_effects_disabled())
		m_prot_read_index++;

	if (BIT(prot_read_index, 0))
		return 0xff; //value is discarded

	return wing_str[(prot_read_index >> 1)-1];
}

void pinkiri8_state::ronjan_prot_w(uint8_t data)
{
	if (data == 0)
	{
		m_prot_index = 0;
	}
	else
	{
		if (m_prot_index == 5)
			return;

		m_prot_char[m_prot_index++] = data;

		if (m_prot_char[0] == 'E' && m_prot_char[1] == 'R' && m_prot_char[2] == 'R' && m_prot_char[3] == 'O' && m_prot_char[4] == 'R')
			m_prot_read_index = 0;
	}
}

uint8_t pinkiri8_state::ronjan_prot_status_r()
{
	return 0; //bit 0 seems a protection status bit
}

uint8_t pinkiri8_state::ronjan_patched_prot_r()
{
	return 0; //value is read then discarded
}

GAME( 1992,  janshi,   0,       janshi,   janshi,   pinkiri8_state, empty_init, ROT0, "Eagle",         "Janshi",                MACHINE_IMPERFECT_SOUND | MACHINE_IMPERFECT_GRAPHICS | MACHINE_NOT_WORKING )
GAME( 1991,  ronjan,   ronjans, ronjan,   ronjan,   pinkiri8_state, empty_init, ROT0, "Wing Co., Ltd", "Ron Jan (set 1)",       MACHINE_IMPERFECT_SOUND | MACHINE_IMPERFECT_GRAPHICS | MACHINE_NOT_WORKING )
GAME( 1994,  ronjana,  ronjans, ronjan,   ronjan,   pinkiri8_state, empty_init, ROT0, "Wing Co., Ltd", "Ron Jan (set 2)",       MACHINE_IMPERFECT_SOUND | MACHINE_IMPERFECT_GRAPHICS | MACHINE_NOT_WORKING )
GAME( 1994,  ronjans,  0,       ronjan,   ronjan,   pinkiri8_state, empty_init, ROT0, "Wing Co., Ltd", "Ron Jan Super (set 1)", MACHINE_IMPERFECT_SOUND | MACHINE_IMPERFECT_GRAPHICS | MACHINE_NOT_WORKING ) // 'SUPER' flashes in the middle of the screen
GAME( 1994,  ronjansa, ronjans, ronjan,   ronjan,   pinkiri8_state, empty_init, ROT0, "Wing Co., Ltd", "Ron Jan Super (set 2)", MACHINE_IMPERFECT_SOUND | MACHINE_IMPERFECT_GRAPHICS | MACHINE_NOT_WORKING ) // possibly Super or not, needs internal ROM dump
GAME( 1994,  ronjansb, ronjans, ronjan,   ronjan,   pinkiri8_state, empty_init, ROT0, "Wing Co., Ltd", "Ron Jan Super (set 3)", MACHINE_IMPERFECT_SOUND | MACHINE_IMPERFECT_GRAPHICS | MACHINE_NOT_WORKING ) // "
GAME( 1990,  janmu,    0,       ronjan,   ronjan,   pinkiri8_state, empty_init, ROT0, "Wing Co., Ltd", "Jan Mu",                MACHINE_IMPERFECT_SOUND | MACHINE_IMPERFECT_GRAPHICS | MACHINE_NOT_WORKING ) // needs internal ROM dump
GAME( 1994,  pinkiri8, 0,       pinkiri8, pinkiri8, pinkiri8_state, empty_init, ROT0, "Alta",          "Pinkiri 8",             MACHINE_NO_SOUND | MACHINE_IMPERFECT_GRAPHICS | MACHINE_NOT_WORKING )
