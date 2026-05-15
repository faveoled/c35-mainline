// SPDX-License-Identifier: GPL-2.0-only
// Based on command information from the vendor device tree

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/reset.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

struct nt36672c_panel {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct gpio_desc *reset_gpio;
};

static inline
struct nt36672c_panel *to_nt36672c_panel(struct drm_panel *panel)
{
	return container_of(panel, struct nt36672c_panel, panel);
}

static int nt36672c_panel_on(struct nt36672c_panel *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;

	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xff, 0x10);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb0, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc1, 0x89, 0x28, 0x00, 0x08, 0x00, 0xaa, 0x02, 0x0e, 0x00, 0x2b, 0x00, 0x07, 0x0d, 0xb7, 0x0c, 0xb7);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xc2, 0x1b, 0xa0);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x3b, 0x03, 0x14, 0x36, 0x04, 0x04);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xff, 0x25);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x18, 0x22);

	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xff, 0x2a);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x1c, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x1d, 0x36);

	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xff, 0xe0);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x35, 0x82);

	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xff, 0xf0);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x1c, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x33, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x5a, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xd2, 0x52);

	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xff, 0xd0);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x53, 0x22);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x54, 0x02);

	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xff, 0xc0);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x9c, 0x11);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x9d, 0x11);


	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xff, 0x10);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xfb, 0x01);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x35, 0x00);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x68, 0x01, 0x01);

	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x53, 0x2c);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0x55, 0x00);

	mipi_dsi_dcs_exit_sleep_mode_multi(&dsi_ctx);
	msleep(120);

	mipi_dsi_dcs_set_display_on_multi(&dsi_ctx);
	msleep(20);

	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x51, 0x00, 0x80); // brightness-levels
	
	return dsi_ctx.accum_err;
}

static void nt36672c_panel_reset(struct nt36672c_panel *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(6000, 7000);

	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(6000, 7000);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(6000, 7000);

	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(6000, 7000);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(11000, 12000);
}


static int nt36672c_panel_prepare(struct drm_panel *panel)
{
	struct nt36672c_panel *ctx = to_nt36672c_panel(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	nt36672c_panel_reset(ctx);

	ret = nt36672c_panel_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		return ret;
	}

	return 0;
}

static int nt36672c_panel_off(struct nt36672c_panel *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	mipi_dsi_dcs_set_display_off_multi(&dsi_ctx);
	mipi_dsi_dcs_enter_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 120);

	return dsi_ctx.accum_err;
}

static int nt36672c_panel_unprepare(struct drm_panel *panel)
{
	struct nt36672c_panel *ctx = to_nt36672c_panel(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = nt36672c_panel_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);

	return 0;
}

static const struct drm_display_mode nt36672c_panel_mode = {
	.clock = 193000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 132,
	.hsync_end = 1080 + 132 + 16,
	.htotal = 1080 + 132 + 16 + 68,
	.vdisplay = 2408,
	.vsync_start = 2408 + 12,
	.vsync_end = 2408 + 12 + 8,
	.vtotal = 2408 + 12 + 8 + 54,
	.width_mm = 67,
	.height_mm = 153,
	.type = DRM_MODE_TYPE_DRIVER,
};

static int nt36672c_panel_get_modes(struct drm_panel *panel,
				       struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &nt36672c_panel_mode);
}

static const struct drm_panel_funcs nt36672c_panel_panel_funcs = {
	.prepare = nt36672c_panel_prepare,
	.unprepare = nt36672c_panel_unprepare,
	.get_modes = nt36672c_panel_get_modes,
};

static int nt36672c_panel_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct nt36672c_panel *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset_gpio))
	    return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio), "Failed to get reset GPIO\n");
    
	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST;
	dsi->hs_rate = 1158000000;
	dsi->lp_rate = 20000000;

	drm_panel_init(&ctx->panel, dev, &nt36672c_panel_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);
	ctx->panel.prepare_prev_first = true;

	ret = drm_panel_of_backlight(&ctx->panel);
	if (ret) {
		printk(KERN_INFO "Hello 1\n");
		return dev_err_probe(dev, ret, "Failed to get backlight\n");
	}
	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		printk(KERN_INFO "Hello 2\n");	
		drm_panel_remove(&ctx->panel);
		return dev_err_probe(dev, ret, "Failed to attach to DSI host\n");
	}
	printk(KERN_INFO "Hello 3 (Probed)!\n");
	return 0;
}

static void nt36672c_panel_remove(struct mipi_dsi_device *dsi)
{
	struct nt36672c_panel *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id nt36672c_panel_of_match[] = {
	{ .compatible = "novatek,nt36672c-1080x2408-panel" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, nt36672c_panel_of_match);

static struct mipi_dsi_driver nt36672c_panel_driver = {
	.probe = nt36672c_panel_probe,
	.remove = nt36672c_panel_remove,
	.driver = {
		.name = "panel-nt36672c-1080x2408",
		.of_match_table = nt36672c_panel_of_match,
	},
};
module_mipi_dsi_driver(nt36672c_panel_driver);

MODULE_AUTHOR("faveoled <faveoled@yandex.com>");
MODULE_DESCRIPTION("DRM driver for nt36672c 1080x2408 video mode DSI panel");
MODULE_LICENSE("GPL");
