/* $NetBSD$ */

/*-
 * Copyright (c) 2025 Rui-Xiang Guo
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <dev/fdt/fdtvar.h>

static int ti_comp_clock_match(device_t, cfdata_t, void *);
static void ti_comp_clock_attach(device_t, device_t, void *);

static struct clk *ti_comp_clock_decode(device_t, int, const void *, size_t);

static const struct fdtbus_clock_controller_func ti_comp_clock_fdt_funcs = {
	.decode = ti_comp_clock_decode
};

static struct clk *ti_comp_clock_get(void *, const char *);
static void ti_comp_clock_put(void *, struct clk *);
static u_int ti_comp_clock_get_rate(void *, struct clk *);
static int ti_comp_clock_enable(void *, struct clk *);
static int ti_comp_clock_disable(void *, struct clk *);
static struct clk *ti_comp_clock_get_parent(void *, struct clk *);
static int ti_comp_clock_set_parent(void *, struct clk *, struct clk *);

static const struct clk_funcs ti_comp_clock_clk_funcs = {
	.get = ti_comp_clock_get,
	.put = ti_comp_clock_put,
	.get_rate = ti_comp_clock_get_rate,
	.enable = ti_comp_clock_enable,
	.disable = ti_comp_clock_disable,
	.get_parent = ti_comp_clock_get_parent,
	.set_parent = ti_comp_clock_set_parent,
};

struct ti_comp_clock_softc {
	device_t		sc_dev;
	int			sc_phandle;

	struct clk_domain	sc_clkdom;
	struct clk		sc_clk;
};

#define	RD4(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

CFATTACH_DECL_NEW(ti_comp_clock, sizeof(struct ti_comp_clock_softc),
    ti_comp_clock_match, ti_comp_clock_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,composite-clock" },
	DEVICE_COMPAT_EOL
};

static int
ti_comp_clock_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct fdt_attach_args *faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
ti_comp_clock_attach(device_t parent, device_t self, void *aux)
{
	struct ti_comp_clock_softc * const sc = device_private(self);
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;

	sc->sc_clkdom.name = device_xname(self);
	sc->sc_clkdom.funcs = &ti_comp_clock_clk_funcs;
	sc->sc_clkdom.priv = sc;

	sc->sc_clk.domain = &sc->sc_clkdom;
	sc->sc_clk.name = kmem_asprintf("%s", faa->faa_name);
	clk_attach(&sc->sc_clk);

	aprint_naive("\n");
	aprint_normal(": TI composite clock (%s)\n", sc->sc_clk.name);

	fdtbus_register_clock_controller(self, phandle, &ti_comp_clock_fdt_funcs);
}

static struct clk *
ti_comp_clock_decode(device_t dev, int cc_phandle, const void *data,
		     size_t len)
{
	struct ti_comp_clock_softc * const sc = device_private(dev);

	return &sc->sc_clk;
}

static struct clk *
ti_comp_clock_get(void *priv, const char *name)
{
	struct ti_comp_clock_softc * const sc = priv;

	return &sc->sc_clk;
}

static void
ti_comp_clock_put(void *priv, struct clk *clk)
{
}

static u_int
ti_comp_clock_get_rate(void *priv, struct clk *clk)
{
	struct clk *clk_parent = clk_get_parent(clk);

	if (clk_parent == NULL)
		return 0;

	return clk_get_rate(clk_parent);
}

static int
ti_comp_clock_enable(void *priv, struct clk *clk)
{
	struct ti_comp_clock_softc * const sc = priv;
	struct clk *clk_gate = fdtbus_clock_get_index(sc->sc_phandle, 0);

	return clk_enable(clk_gate);
}

static int
ti_comp_clock_disable(void *priv, struct clk *clk)
{
	struct ti_comp_clock_softc * const sc = priv;
	struct clk *clk_gate = fdtbus_clock_get_index(sc->sc_phandle, 0);

	return clk_disable(clk_gate);
}

static struct clk *
ti_comp_clock_get_parent(void *priv, struct clk *clk)
{
	struct ti_comp_clock_softc * const sc = priv;

	return fdtbus_clock_get_index(sc->sc_phandle, 1);
}

static int
ti_comp_clock_set_parent(void *priv, struct clk *clk, struct clk *parent_clk)
{
	return clk_set_parent(clk, parent_clk);
}
