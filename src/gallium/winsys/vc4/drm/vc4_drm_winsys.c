/*
 * Copyright © 2014 Broadcom
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <unistd.h>
#include <fcntl.h>

#include "renderonly/renderonly.h"
#include "vc4_drm_public.h"
#include "vc4/vc4_screen.h"
#include "util/u_screen.h"

struct pipe_screen *
vc4_drm_screen_create(int fd)
{
   int dupfd;
   struct pipe_screen *pscreen = pipe_screen_reference(fd);
   if (pscreen)
      return pscreen;

   dupfd = fcntl(fd, F_DUPFD_CLOEXEC, 3);
   pscreen = vc4_screen_create(dupfd, NULL);
   pipe_screen_reference_init(pscreen, dupfd);
   return pscreen;
}

struct pipe_screen *
vc4_drm_screen_create_renderonly(struct renderonly *ro)
{
   int dupfd;
   struct pipe_screen *pscreen = pipe_screen_reference(ro->gpu_fd);
   if (pscreen)
      return pscreen;

   dupfd = fcntl(ro->gpu_fd, F_DUPFD_CLOEXEC, 3);
   pscreen = vc4_screen_create(dupfd, ro);
   pipe_screen_reference_init(pscreen, dupfd);
   return pscreen;
}
