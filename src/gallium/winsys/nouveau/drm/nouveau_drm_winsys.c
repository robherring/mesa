#include <unistd.h>
#include <fcntl.h>
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_screen.h"

#include "nouveau_drm_public.h"

#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_screen.h"

#include <nvif/class.h>
#include <nvif/cl0080.h>

PUBLIC struct pipe_screen *
nouveau_drm_screen_create(int fd)
{
	struct nouveau_drm *drm = NULL;
	struct nouveau_device *dev = NULL;
	struct nouveau_screen *(*init)(struct nouveau_device *);
	struct nouveau_screen *screen = NULL;
	struct pipe_screen *pscreen = pipe_screen_reference(fd);
	int ret, dupfd;

	if (pscreen)
		return pscreen;

	/* Since the screen re-use is based on the device node and not the fd,
	 * create a copy of the fd to be owned by the device. Otherwise a
	 * scenario could occur where two screens are created, and the first
	 * one is shut down, along with the fd being closed. The second
	 * (identical) screen would now have a reference to the closed fd. We
	 * avoid this by duplicating the original fd. Note that
	 * nouveau_device_wrap does not close the fd in case of a device
	 * creation error.
	 */
	dupfd = fcntl(fd, F_DUPFD_CLOEXEC, 3);

	ret = nouveau_drm_new(dupfd, &drm);
	if (ret)
		goto err;

	ret = nouveau_device_new(&drm->client, NV_DEVICE,
				 &(struct nv_device_v0) {
					.device = ~0ULL,
				 }, sizeof(struct nv_device_v0), &dev);
	if (ret)
		goto err;

	switch (dev->chipset & ~0xf) {
	case 0x30:
	case 0x40:
	case 0x60:
		init = nv30_screen_create;
		break;
	case 0x50:
	case 0x80:
	case 0x90:
	case 0xa0:
		init = nv50_screen_create;
		break;
	case 0xc0:
	case 0xd0:
	case 0xe0:
	case 0xf0:
	case 0x100:
	case 0x110:
	case 0x120:
	case 0x130:
		init = nvc0_screen_create;
		break;
	default:
		debug_printf("%s: unknown chipset nv%02x\n", __func__,
			     dev->chipset);
		goto err;
	}

	screen = init(dev);
	if (!screen || !screen->base.context_create)
		goto err;

	/* Use dupfd in hash table, to avoid errors if the original fd gets
	 * closed by its owner. The hash key needs to live at least as long as
	 * the screen.
	 */
	pipe_screen_reference_init(&screen->base, dupfd);
	return &screen->base;

err:
	if (screen) {
		screen->base.destroy(&screen->base);
	} else {
		nouveau_device_del(&dev);
		nouveau_drm_del(&drm);
		close(dupfd);
	}
	return NULL;
}
