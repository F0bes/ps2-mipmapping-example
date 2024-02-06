#include <stdlib.h>
#include <stdio.h>

#include <kernel.h>
#include <dma.h>
#include <graph.h>
#include <draw.h>

#include <gs_gp.h>
#include <gs_psm.h>

#include "mip_level_defines.h"

framebuffer_t fb =
	{
		.width = 640,
		.height = 448,
		.mask = 0,
		.psm = GS_PSM_24,
		.address = 0x00};

zbuffer_t zb =
	{
		.enable = 1,
		.method = ZTEST_METHOD_ALLPASS,
		.zsm = GS_PSMZ_24,
		.mask = 0,
		.address = 0x00};

// Nothing special here, just setting up the framebuffer and zbuffer and the draw context
void setup_gs()
{
	fb.address = graph_vram_allocate(fb.width, fb.height, fb.psm, GRAPH_ALIGN_PAGE);
	zb.address = graph_vram_allocate(fb.width, fb.height, zb.zsm, GRAPH_ALIGN_PAGE);
	printf("Allocated framebuffer at %x\n", fb.address);
	graph_initialize(fb.address, fb.width, fb.height, fb.psm, 0, 0);

	qword_t *draw_context_pk = aligned_alloc(16, sizeof(qword_t) * 15);
	qword_t *q = draw_context_pk;

	PACK_GIFTAG(q, GIF_SET_TAG(14, 1, GIF_PRE_DISABLE, 0, GIF_FLG_PACKED, 1),
				GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_FRAME(fb.address >> 11, fb.width >> 6, fb.psm, 0), GS_REG_FRAME);
	q++;
	PACK_GIFTAG(q, GS_SET_TEST(1, 7, 0, 2, 0, 0, 1, 1), GS_REG_TEST);
	q++;
	PACK_GIFTAG(q, GS_SET_PABE(0), GS_REG_PABE);
	q++;
	PACK_GIFTAG(q, GS_SET_ALPHA(0, 1, 0, 1, 128), GS_REG_ALPHA);
	q++;
	PACK_GIFTAG(q, GS_SET_ZBUF(zb.address >> 11, zb.zsm, 0), GS_REG_ZBUF);
	q++;
	PACK_GIFTAG(q, GS_SET_XYOFFSET(0, 0), GS_REG_XYOFFSET);
	q++;
	PACK_GIFTAG(q, GS_SET_DTHE(0), GS_REG_DTHE);
	q++;
	PACK_GIFTAG(q, GS_SET_PRMODECONT(1), GS_REG_PRMODECONT);
	q++;
	PACK_GIFTAG(q, GS_SET_SCISSOR(0, fb.width - 1, 0, fb.height - 1), GS_REG_SCISSOR);
	q++;
	PACK_GIFTAG(q, GS_SET_CLAMP(0, 0, 0, 0, 0, 0), GS_REG_CLAMP);
	q++;
	PACK_GIFTAG(q, GS_SET_SCANMSK(0), GS_REG_SCANMSK);
	q++;
	PACK_GIFTAG(q, GS_SET_TEXA(0x00, 0, 0x7F), GS_REG_TEXA);
	q++;
	PACK_GIFTAG(q, GS_SET_FBA(0), GS_REG_FBA);
	q++;
	PACK_GIFTAG(q, GS_SET_COLCLAMP(GS_ENABLE), GS_REG_COLCLAMP);
	q++;

	FlushCache(0);
	dma_channel_send_normal(DMA_CHANNEL_GIF, draw_context_pk, q - draw_context_pk, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	free(draw_context_pk);
}

void clear_frame()
{
	qword_t *draw_pk = aligned_alloc(16, sizeof(qword_t) * 4);
	qword_t *q = draw_pk;

	PACK_GIFTAG(q, GIF_SET_TAG(3, 1, GIF_PRE_ENABLE, GS_PRIM_SPRITE, GIF_FLG_PACKED, 1),
				GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_RGBAQ(0x00, 0x44, 0x00, 0x7f, 0), GS_REG_RGBAQ);
	q++;
	PACK_GIFTAG(q, GS_SET_XYZ(0, 0, 0), GS_REG_XYZ2);
	q++;
	PACK_GIFTAG(q, GS_SET_XYZ(640 << 4, 448 << 4, 0), GS_REG_XYZ2);
	q++;

	FlushCache(0);
	dma_channel_send_normal(DMA_CHANNEL_GIF, draw_pk, q - draw_pk, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	free(draw_pk);
}

u32 mip_level_address[5];
void upload_mips()
{
	u32 mip_size = 128;
	for (int i = 0; i < 5; i++)
	{
		mip_level_address[i] = graph_vram_allocate(mip_size, mip_size, GS_PSM_32, GRAPH_ALIGN_BLOCK);
		printf("Allocated mip level %d (size: %d) at address %x\n", i, mip_size, mip_level_address[i]);
		mip_size >>= 1;
	}

	qword_t *transfer_pk = aligned_alloc(16, sizeof(qword_t) * 100);
	qword_t *q = transfer_pk;
	q = draw_texture_transfer(q, mip_level_0, 128, 128, GS_PSM_32, mip_level_address[0], 128);
	q = draw_texture_transfer(q, mip_level_1, 64, 64, GS_PSM_32, mip_level_address[1], 64);
	q = draw_texture_transfer(q, mip_level_2, 32, 32, GS_PSM_32, mip_level_address[2], 64);
	q = draw_texture_transfer(q, mip_level_3, 16, 16, GS_PSM_32, mip_level_address[3], 64);
	q = draw_texture_transfer(q, mip_level_4, 8, 8, GS_PSM_32, mip_level_address[4], 64);
	q = draw_texture_flush(q);

	FlushCache(0);
	dma_channel_send_chain(DMA_CHANNEL_GIF, transfer_pk, q - transfer_pk, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	free(transfer_pk);
}

void draw_mips()
{
	qword_t *draw_pk = aligned_alloc(16, sizeof(qword_t) * 50);
	qword_t *q = draw_pk;

	PACK_GIFTAG(q, GIF_SET_TAG(33, 1, GIF_PRE_ENABLE, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 0, 0, 0, 0, 0), GIF_FLG_PACKED, 1),
				GIF_REG_AD);
	q++;
	// Set our level 0 mip texture. Everything here is exactly the same as it would be without mipmap levels
	PACK_GIFTAG(q, GS_SET_TEX0(mip_level_address[0] >> 6, 2, GS_PSM_32, 7, 7, 0, 1, 0, 0, 0, 0, 0), GS_REG_TEX0);
	q++;
	// Set the location of each mipmap level
	// You can enable MTBA in TEX1 to automatically calculate the addresses
	// But that's not something I want to get into right now :)
	PACK_GIFTAG(q, GS_SET_MIPTBP1(mip_level_address[1] >> 6, 1, mip_level_address[2] >> 6, 1, mip_level_address[3] >> 6, 1), GS_REG_MIPTBP1);
	q++;
	PACK_GIFTAG(q, GS_SET_MIPTBP2(mip_level_address[4] >> 6, 1, 0, 0, 0, 0), GS_REG_MIPTBP2);
	q++;

	int xStart = 0;
	int yStart = 0;
	for (int i = 0; i < 5; i++)
	{
		// LCM = 1
		//     This allow us to manually set the mipmap level using the K value in this register
		//     LCM = 0 would set the mipmap level to the result of a calculation. You can see the formula in the GS manual
		// MXL = 4
		//     This is the maximum mipmap level. We have 5 levels, so the max is 4.
		// MMAG = 0
		//     This allows us to enable bilinear when LOD < 0. It is not used in this example
		// MMIN = 3
		//     This allows us to enable bilinear when LOD >= 0. See page 61 (3.4.12) of the GS manual for more details
		//     The important part is that it's > 1, so mipmapping is enabled
		// MTBA = 0
		//     As explained above, this would allow us to automatically calculate the addresses of the mipmap levels.
		// L = 0
		//     This is used in the formula when LCM = 0.
		// K = i
		//     This is used both when LCM = 0 and LCM = 1.
		//     Because LCM = 1, this means our LOD = K
		PACK_GIFTAG(q, GS_SET_TEX1(1, 4, 0, 3, 0, 0, i << 4), GS_REG_TEX1);
		q++;
		// Ensure that you set Q to 1 before writing to ST
		PACK_GIFTAG(q, GS_SET_RGBAQ(0x00, 0x00, 0xff, 0x7F, 0x3f800000), GS_REG_RGBAQ);
		q++;
		PACK_GIFTAG(q, GS_SET_ST(0x00, 0x00), GS_REG_ST);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ(xStart << 4, yStart << 4, 0), GS_REG_XYZ2);
		q++;
		PACK_GIFTAG(q, GS_SET_ST(0x3f800000, 0x3f800000), GS_REG_ST);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ((xStart + 128) << 4, (yStart + 128) << 4, 0), GS_REG_XYZ2);
		q++;

		xStart += 128;
		if (xStart >= 512)
		{
			xStart = 0;
			yStart += 128;
		}
	}

	FlushCache(0);
	dma_channel_send_normal(DMA_CHANNEL_GIF, draw_pk, q - draw_pk, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	free(draw_pk);
}

u32 vsync_sema_id = 0;

s32 vsync_handler()
{
	iSignalSema(vsync_sema_id);
	ExitHandler();
	return 0;
}

int main(void)
{
	ee_sema_t vsync_sema =
		{
			.init_count = 0,
			.max_count = 1,
			.count = 0,
			.option = 0,
			.wait_threads = 1,
			.attr = 0};

	vsync_sema_id = CreateSema(&vsync_sema);

	u32 vsync_handler_id = AddIntcHandler(INTC_VBLANK_S, vsync_handler, -1);
	EnableIntc(INTC_VBLANK_S);

	setup_gs();

	upload_mips();
	// Main loop
	while (1)
	{
		clear_frame();
		draw_mips();

		WaitSema(vsync_sema_id);
	}

	RemoveIntcHandler(INTC_VBLANK_S, vsync_handler_id);
	DeleteSema(vsync_sema_id);

	SleepThread();
}
