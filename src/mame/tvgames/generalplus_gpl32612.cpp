// license:BSD-3-Clause
// copyright-holders:David Haywood


/*****************************************************************************

    unlike earlier SunPlus / GeneralPlus based SoCs this one is
    ARM based

    NAND types

    MX30LF1G08AA
    ID = C2F1
    Capacity = (2048+64) x 64 x 512

    Star Wars Blaster - MX30LF1G08AA
    TMNT Hero Portal  - MX30LF1G08AA
    DC Hero Portal    - MX30LF1G08AA

*****************************************************************************/


#include "emu.h"

#include "cpu/arm7/arm7.h"
#include "bus/generic/slot.h"
#include "bus/generic/carts.h"
#include "softlist_dev.h"

#include "screen.h"
#include "speaker.h"


namespace {

class generalplus_gpl32612_game_state : public driver_device
{
public:
	generalplus_gpl32612_game_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_screen(*this, "screen")
	{ }

	void gpl32612(machine_config &config);

	void nand_init840();
	void nand_init880();

protected:
	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;

	void arm_map(address_map &map) ATTR_COLD;

	required_device<cpu_device> m_maincpu;
	required_device<screen_device> m_screen;

	uint32_t screen_update_gpl32612(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

private:

	void nand_init(int blocksize, int blocksize_stripped);
	void copy_block(int i, int blocksize, int blocksize_stripped, uint8_t* nandrom, int dest);
	void bootstrap();

	std::vector<uint8_t> m_strippedrom;

	uint32_t unk_d000003c_r(offs_t offset, uint32_t mem_mask);
	uint32_t unk_d0800018_r(offs_t offset, uint32_t mem_mask);
	uint32_t unk_d0900140_r(offs_t offset, uint32_t mem_mask);
	uint32_t unk_d0900153_r(offs_t offset, uint32_t mem_mask);
};


class generalplus_zippity_game_state : public generalplus_gpl32612_game_state
{
public:
	generalplus_zippity_game_state(const machine_config &mconfig, device_type type, const char *tag) :
		generalplus_gpl32612_game_state(mconfig, type, tag),
		m_cart(*this, "cartslot"),
		m_cart_region(nullptr)
	{ }

	void zippity(machine_config &config);

protected:
	virtual void machine_start() override ATTR_COLD;

	DECLARE_DEVICE_IMAGE_LOAD_MEMBER(cart_load);

	required_device<generic_slot_device> m_cart;
	memory_region *m_cart_region;


};


uint32_t generalplus_gpl32612_game_state::unk_d000003c_r(offs_t offset, uint32_t mem_mask)
{
	return machine().rand();
}

uint32_t generalplus_gpl32612_game_state::unk_d0800018_r(offs_t offset, uint32_t mem_mask)
{
	return machine().rand();
}

uint32_t generalplus_gpl32612_game_state::unk_d0900140_r(offs_t offset, uint32_t mem_mask)
{
	return machine().rand();
}

uint32_t generalplus_gpl32612_game_state::unk_d0900153_r(offs_t offset, uint32_t mem_mask)
{
	return machine().rand();
}

void generalplus_gpl32612_game_state::arm_map(address_map &map)
{
	map(0x00000000, 0x03ffffff).ram();

	map(0xd000003c, 0xd000003f).r(FUNC(generalplus_gpl32612_game_state::unk_d000003c_r));

	map(0xd0800018, 0xd080001b).r(FUNC(generalplus_gpl32612_game_state::unk_d0800018_r));

	map(0xd0900140, 0xd0900143).r(FUNC(generalplus_gpl32612_game_state::unk_d0900140_r));
	map(0xd0900150, 0xd0900153).r(FUNC(generalplus_gpl32612_game_state::unk_d0900153_r));
}

uint32_t generalplus_gpl32612_game_state::screen_update_gpl32612(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	return 0;
}

void generalplus_gpl32612_game_state::machine_start()
{
}

void generalplus_gpl32612_game_state::machine_reset()
{
}

static INPUT_PORTS_START( gpl32612 )
INPUT_PORTS_END

void generalplus_gpl32612_game_state::copy_block(int i, int blocksize, int blocksize_stripped, uint8_t* nandrom, int dest)
{
	const int base = i * blocksize;
	address_space& mem = m_maincpu->space(AS_PROGRAM);

	for (int j = 0; j < blocksize_stripped; j++)
	{
		uint8_t data = nandrom[base + j];
		//printf("writing to %08x : %02x", dest + j, data);
		mem.write_byte(dest+j, data);
	}
}

void generalplus_gpl32612_game_state::bootstrap()
{
	uint8_t* rom = memregion("nand")->base();

	//int startblock = 0xe0000 / 0x800;
	int startblock = 0xa0000 / 0x800;
	int endblock = 0x1f0000 / 0x800;

	int j = 0;
	for (int i = startblock; i < endblock; i++) // how much is copied, and where from? as with the unSP NAND ones there appear to be several stages of bootloader, this is not the 1st one
	{
		copy_block(i, 0x840, 0x800, rom, j * 0x800);
		j++;
	}
}

void generalplus_gpl32612_game_state::gpl32612(machine_config &config)
{
	ARM9(config, m_maincpu, 240'000'000); // unknown core / frequency, but ARM based
	m_maincpu->set_addrmap(AS_PROGRAM, &generalplus_gpl32612_game_state::arm_map);

	SCREEN(config, m_screen, SCREEN_TYPE_RASTER);
	m_screen->set_refresh_hz(60);
	m_screen->set_size(320, 262);
	m_screen->set_visarea(0, 320-1, 0, 240-1);
	m_screen->set_screen_update(FUNC(generalplus_gpl32612_game_state::screen_update_gpl32612));

	SPEAKER(config, "speaker", 2).front();
}

void generalplus_zippity_game_state::machine_start()
{
	generalplus_gpl32612_game_state::machine_start();

	// if there's a cart, override the standard mapping
	if (m_cart && m_cart->exists())
	{
		m_cart_region = memregion(std::string(m_cart->tag()) + GENERIC_ROM_REGION_TAG);
	}
}

DEVICE_IMAGE_LOAD_MEMBER(generalplus_zippity_game_state::cart_load)
{
	uint32_t const size = m_cart->common_get_size("rom");

	m_cart->rom_alloc(size, GENERIC_ROM16_WIDTH, ENDIANNESS_LITTLE);
	m_cart->common_load_rom(m_cart->get_rom_base(), size, "rom");

	return std::make_pair(std::error_condition(), std::string());
}



void generalplus_zippity_game_state::zippity(machine_config &config)
{
	// don't inherit from gpl32612 as in reality this is GPL32300A and could differ

	ARM9(config, m_maincpu, 240'000'000); // unknown core / frequency, but ARM based
	m_maincpu->set_addrmap(AS_PROGRAM, &generalplus_zippity_game_state::arm_map);

	SCREEN(config, m_screen, SCREEN_TYPE_RASTER);
	m_screen->set_refresh_hz(60);
	m_screen->set_size(320, 262);
	m_screen->set_visarea(0, 320-1, 0, 240-1);
	m_screen->set_screen_update(FUNC(generalplus_zippity_game_state::screen_update_gpl32612));

	SPEAKER(config, "speaker", 2).front();

	GENERIC_CARTSLOT(config, m_cart, generic_plain_slot, "leapfrog_zippity_cart");
	m_cart->set_width(GENERIC_ROM16_WIDTH);
	m_cart->set_device_load(FUNC(generalplus_zippity_game_state::cart_load));

	SOFTWARE_LIST(config, "cart_list").set_original("leapfrog_zippity_cart");
}



// NAND dumps, so there will be a bootloader / boot strap at least

ROM_START( jak_swbstrik )
	ROM_REGION( 0x8400000, "nand", ROMREGION_ERASEFF )
	ROM_LOAD( "starwarsblaster.bin", 0x000000, 0x8400000, CRC(02c3c4d6) SHA1(a6ae05a7d7b2015023113f6baad25458f3c01102) )
ROM_END

ROM_START( jak_tmnthp )
	ROM_REGION( 0x8400000, "nand", ROMREGION_ERASEFF )
	ROM_LOAD( "tmntheroportal.bin", 0x000000, 0x8400000, CRC(75ec7127) SHA1(cd05f55a1f5a7fd3d1b0658ad6805b8777857a7e) )
ROM_END

ROM_START( jak_ddhp )
	ROM_REGION( 0x8400000, "nand", ROMREGION_ERASEFF )
	ROM_LOAD( "dragonsheroesportal_mx30lf1g08aa_c2f1.bin", 0x000000, 0x8400000, CRC(825cce7b) SHA1(2185137138f2a20e5cfe9c167eeb67a146953b65) )
ROM_END

ROM_START( jak_dchp )
	ROM_REGION( 0x8400000, "nand", ROMREGION_ERASEFF )
	ROM_LOAD( "dcheroportal_mx30lf1g08aa_c2f1.bin", 0x000000, 0x8400000, CRC(576a3005) SHA1(6cd9edc4def707aede3f82a21c87269d2a6bc870) )
ROM_END

ROM_START( jak_prhp )
	ROM_REGION( 0x8400000, "nand", ROMREGION_ERASEFF )
	ROM_LOAD( "mx30lf1g08aa.u2", 0x000000, 0x8400000, CRC(4ccd7e53) SHA1(decbd424f088d180776a817c80b147d6a887e5c1) )
ROM_END

// uncertain hardware type, ARM based, has GPNAND strings
ROM_START( zippity )
	ROM_REGION( 0x10800000, "nand", ROMREGION_ERASE00 )
	ROM_LOAD( "zippity_mt29f2g08aacwp_2cda8015.bin", 0x0000, 0x10800000, CRC(16248b63) SHA1(3607337588a68052ef5c495b496aa3e0449d3eb6) )
ROM_END

ROM_START( zippityuk )
	ROM_REGION( 0x10800000, "nand", ROMREGION_ERASE00 )
	ROM_LOAD( "29f2c08aacwp.u2", 0x0000, 0x10800000, CRC(27d172ae) SHA1(9ade19d7aa28fba13581e6879b39e3a7702260b0) )
ROM_END

ROM_START( kidizmp )
	ROM_REGION( 0x10800000, "nand", ROMREGION_ERASE00 )
	ROM_LOAD( "s34ml02g1_withspare.u13", 0x0000, 0x10800000, CRC(c5d55bdc) SHA1(073fc3fd56c532750b4e2020abe27d3448999d56) )
ROM_END

ROM_START( kidizmb )
	ROM_REGION(  0x8400000, "nand", ROMREGION_ERASE00 )
	ROM_LOAD( "hy27uf081g2a_withspare.bin", 0x0000, 0x8400000, CRC(b87861c4) SHA1(8b5cc2557b54a37928be818430b91c48db98758f) )
ROM_END

ROM_START( pocketmp )
	ROM_REGION(  0x8800000, "nand", ROMREGION_ERASE00 )
	ROM_LOAD( "tc58nvg0s3hta00.u3", 0x0000, 0x8800000, CRC(aabf2deb) SHA1(ee3118377c21b1fb28ff262484c9b587b394bd80) )
ROM_END

ROM_START( pocketmr )
	ROM_REGION(  0x8800000, "nand", ROMREGION_ERASE00 )
	ROM_LOAD( "tc58nvg0s3hta00_withspare.u6", 0x0000, 0x8800000, CRC(ec839dde) SHA1(18b77c7e1cf3c66787ccfde9f450671e3d1b0e36) )
ROM_END

ROM_START( dmnslayg )
	ROM_REGION(  0x8800000, "nand", ROMREGION_ERASE00 )
	ROM_LOAD( "tc58nvg0s3hta00_with_spare.u3", 0x0000, 0x8800000, CRC(a9402fdb) SHA1(0809a8da176f65efc2926131ba0259278d3c644d) )
ROM_END

ROM_START( anpanm19 )
	ROM_REGION(  0x1000000, "spi", ROMREGION_ERASE00 )
	ROM_LOAD( "25l1280.u3", 0x0000, 0x1000000, CRC(7932fb3e) SHA1(a381eeba5357fe71e4d6081b9b91b57e5705f7f1) )
ROM_END

ROM_START( smatomo )
	ROM_REGION(  0x400000, "spi", ROMREGION_ERASE00 )
	ROM_LOAD( "mx25l3206e.ic3", 0x0000, 0x400000, CRC(fb4d1684) SHA1(98cecd7ead52118028cb3a1de71cb3528cd81be5) )
ROM_END

ROM_START( intrtvg )
	ROM_REGION(  0x100000, "spi", ROMREGION_ERASE00 )
	ROM_LOAD( "25q08.u6", 0x0000, 0x100000, CRC(5aa91972) SHA1(296108e8683063c16951ff326e6ff3d63d9ed5b8) )

	DISK_REGION( "sdcard" ) // 4GB SD Card
	DISK_IMAGE( "interactivetv", 0, SHA1(7061e28c4560b763bda1157036b79c726387e430) )
ROM_END

ROM_START( ardancem )
	ROM_REGION(  0x100000, "spi", ROMREGION_ERASE00 )
	ROM_LOAD( "25q08.u6", 0x0000, 0x100000, CRC(ba2cdacd) SHA1(d47829ee5310140665146262a44e0ba91942f25c) )

	DISK_REGION( "sdcard" ) // 16GB SD Card
	DISK_IMAGE( "ardancemat", 0, SHA1(df8cb065f5ce0ca863b205549ecc4c27647f9954) )
ROM_END


ROM_START( pdcm2 )
	ROM_REGION16_BE( 0x40000, "maincpu:internal", ROMREGION_ERASE00 )
	ROM_LOAD16_WORD_SWAP( "internal.rom", 0x00000, 0x40000, NO_DUMP ) // used as bootstrap only?

	ROM_REGION( 0x84000000, "nand", ROMREGION_ERASE00 )
	ROM_LOAD( "k9gag08u0m.u3", 0x0000, 0x84000000, CRC(88d9c107) SHA1(0b70962ecddf3a8a748b7af5e81cffb365f704e2) )
ROM_END

ROM_START( arcadege )
	ROM_REGION16_BE( 0x40000, "maincpu:internal", ROMREGION_ERASE00 )
	ROM_LOAD16_WORD_SWAP( "internal.rom", 0x00000, 0x40000, NO_DUMP ) // used as bootstrap only?

	ROM_REGION( 0x8400000, "nand", ROMREGION_ERASE00 )
	ROM_LOAD( "tc58nvg0s3eta00.u3", 0x0000, 0x8400000, CRC(9b4db25e) SHA1(7e3d7e15f2592efd98027440c3761179c95e4417) )
ROM_END

ROM_START( airobo )
	ROM_REGION16_BE( 0x40000, "maincpu:internal", ROMREGION_ERASE00 )
	ROM_LOAD16_WORD_SWAP( "internal.rom", 0x00000, 0x40000, NO_DUMP ) // used as bootstrap only?

	ROM_REGION( 0x22000000, "nand", ROMREGION_ERASE00 )
	ROM_LOAD( "tc58nvg2s0hta00.bin", 0x0000, 0x22000000, CRC(6cfa4600) SHA1(152c04532ae2587dea590d169e87534924f5ea89) )
ROM_END


void generalplus_gpl32612_game_state::nand_init(int blocksize, int blocksize_stripped)
{
	uint8_t* rom = memregion("nand")->base();
	int size = memregion("nand")->bytes();

	int numblocks = size / blocksize;

	m_strippedrom.resize(numblocks * blocksize_stripped);

	for (int i = 0; i < numblocks; i++)
	{
		const int base = i * blocksize;
		const int basestripped = i * blocksize_stripped;

		for (int j = 0; j < blocksize_stripped; j++)
		{
			m_strippedrom[basestripped + j] = rom[base + j];
		}
	}

	// debug to allow for easy use of unidasm.exe
	if (0)
	{
		auto filename = "stripped_" + std::string(machine().system().name);
		auto fp = fopen(filename.c_str(), "w+b");
		if (fp)
		{
			fwrite(&m_strippedrom[0], blocksize_stripped * numblocks, 1, fp);
			fclose(fp);
		}
	}
}

void generalplus_gpl32612_game_state::nand_init840()
{
	nand_init(0x840, 0x800);
	bootstrap();
}

void generalplus_gpl32612_game_state::nand_init880()
{
	nand_init(0x880, 0x800);
	bootstrap();
}

} // anonymous namespace


//    year, name,         parent,  compat, machine,      input,        class,              init,       company,  fullname,                             flags
CONS( 200?, jak_swbstrik,    0,       0,      gpl32612, gpl32612, generalplus_gpl32612_game_state, nand_init840, "JAKKS Pacific Inc", "Star Wars Blaster Strike", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
CONS( 200?, jak_tmnthp,      0,       0,      gpl32612, gpl32612, generalplus_gpl32612_game_state, nand_init840, "JAKKS Pacific Inc", "Teenage Mutant Ninja Turtles Hero Portal", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
CONS( 200?, jak_ddhp,        0,       0,      gpl32612, gpl32612, generalplus_gpl32612_game_state, nand_init840, "JAKKS Pacific Inc", "DreamWorks Dragons Hero Portal", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
CONS( 200?, jak_prhp,        0,       0,      gpl32612, gpl32612, generalplus_gpl32612_game_state, nand_init840, "JAKKS Pacific Inc", "Power Rangers Super Megaforce Hero Portal", MACHINE_NO_SOUND | MACHINE_NOT_WORKING ) // from a PAL unit (probably not region specific)
CONS( 200?, jak_dchp,        0,       0,      gpl32612, gpl32612, generalplus_gpl32612_game_state, nand_init840, "JAKKS Pacific Inc", "DC Super Heroes The Watchtower Hero Portal", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )

// Might not belong here, SoC is marked GPL32300A instead, but is still ARM based, and has GPNAND strings
CONS( 201?, zippity,         0,       0,      zippity, gpl32612, generalplus_zippity_game_state, empty_init,  "LeapFrog",         "Zippity (US)",   MACHINE_NO_SOUND | MACHINE_NOT_WORKING)
// TODO, check if code differs, or just unused areas of the NAND
CONS( 201?, zippityuk,       zippity, 0,      zippity, gpl32612, generalplus_zippity_game_state, empty_init,  "LeapFrog",         "Zippity (UK)",   MACHINE_NO_SOUND | MACHINE_NOT_WORKING)

// GP32C01 (maybe, picture is unclear) - Camera for kids
CONS( 2013, kidizmp,         0,        0,      gpl32612, gpl32612, generalplus_gpl32612_game_state, empty_init,  "VTech",         "Kidizoom Connect (Germany, pink camera)",   MACHINE_NO_SOUND | MACHINE_NOT_WORKING)
// seems to be older tech, just glob + ROM, assuming it's a GP32 series based on above and due to having ARM code
CONS( 201?, kidizmb,         0,        0,      gpl32612, gpl32612, generalplus_gpl32612_game_state, empty_init,  "VTech",         "Kidizoom (Germany, blue camera)",   MACHINE_NO_SOUND | MACHINE_NOT_WORKING)

CONS( 2019, pocketmp,        0,        0,      gpl32612, gpl32612, generalplus_gpl32612_game_state, nand_init880,  "Takara Tomy",        "Pocket Monsters PC",   MACHINE_NO_SOUND | MACHINE_NOT_WORKING)
CONS( 2019, pocketmr,        0,        0,      gpl32612, gpl32612, generalplus_gpl32612_game_state, nand_init880,  "Takara Tomy",        "Pocket Monsters Rotom Tablet",   MACHINE_NO_SOUND | MACHINE_NOT_WORKING)

// uses GPL32610 - 「それいけ！アンパンマン」スポーツ育脳マット
CONS( 2019, anpanm19,        0,        0,      gpl32612, gpl32612, generalplus_gpl32612_game_state, empty_init,  "JoyPalette",        "Anpanman: Sports Ikunou Mat (Japan)",   MACHINE_NO_SOUND | MACHINE_NOT_WORKING)

// unknown (uses a glob) has GPspispi header, ARM based, SPI ROM
CONS( 201?, smatomo,         0,        0,      gpl32612, gpl32612, generalplus_gpl32612_game_state, empty_init,  "Bandai",        "Smatomo (Japan)",   MACHINE_NO_SOUND | MACHINE_NOT_WORKING)

// very generic packaging, boots from SPI, has game data on SD card (mostly NES games)
CONS( 202?, intrtvg,         0,        0,      gpl32612, gpl32612, generalplus_gpl32612_game_state, empty_init,  "Shen Zhen Shi Fei Xun Hu Dong Technology",     "Interactive Game Console (Model B608, YRPRSODF)",   MACHINE_NO_SOUND | MACHINE_NOT_WORKING)
// also very generic packaging, similar SD card content to above, including NES games, but with some extra music/videos for the dance part
CONS( 202?, ardancem,        0,        0,      gpl32612, gpl32612, generalplus_gpl32612_game_state, empty_init,  "Shen Zhen Shi Fei Xun Hu Dong Technology",     "AR Dance Mat (Model DM02, YRPRSODF)",   MACHINE_NO_SOUND | MACHINE_NOT_WORKING)

// unknown (uses a glob) but it's GeneralPlus and ARM based, so put in here for now
// ROM has 'GPNandTag2' header rather than the usual
// 鬼滅の刃 全集中パッド（グリーン)
CONS( 2021, dmnslayg,        0,        0,      gpl32612, gpl32612, generalplus_gpl32612_game_state, nand_init880,  "Bandai",        "Demon Slayer: Kimetsu no Yaiba Zenshuuchuu Pad (green ver.) (Japan)",   MACHINE_NO_SOUND | MACHINE_NOT_WORKING)

/* PCB is marked as M2-SPG48-GPG35-V30 2009-08-11

SoC appears to be
CONNY CNT61623P-003A-QL172
MD481P
0917

(could be a rebranded GPL32 series, ROM has GPNand header)

there is also a
GPY0201A

*/

CONS( 2009, pdcm2,           0,        0,      gpl32612, gpl32612, generalplus_gpl32612_game_state, empty_init,  "VideoJet / Conny",        "PDC M2",   MACHINE_NO_SOUND | MACHINE_NOT_WORKING)

// uses a GPL32600A-003A-QL141
CONS( 200?, arcadege,           0,        0,      gpl32612, gpl32612, generalplus_gpl32612_game_state, nand_init840,  "Millennium 2000 GmbH",        "Millennium Arcade Genius SE",   MACHINE_NO_SOUND | MACHINE_NOT_WORKING)

// uses a GP326813
// 学習おうえんAI★ミラクルロボ
CONS( 2020, airobo,             0,        0,      gpl32612, gpl32612, generalplus_gpl32612_game_state, nand_init840,  "Benesse Corporation",        "Gakushuu Ouen AI Miracle Robo",   MACHINE_NO_SOUND | MACHINE_NOT_WORKING)
