/*
 * Copyright 2016-2017 Linaro, Ltd., Rob Herring <robh@kernel.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * Functions for managing pipe_screen's
 */

#include <sys/stat.h>

#include "pipe/p_screen.h"
#include "util/u_hash_table.h"
#include "util/u_inlines.h"
#include "util/u_pointer.h"
#include "util/u_screen.h"

static struct util_hash_table *fd_tab = NULL;

static unsigned hash_fd(void *key)
{
   int fd = pointer_to_intptr(key);
   struct stat stat;
   fstat(fd, &stat);

   return stat.st_dev ^ stat.st_ino ^ stat.st_rdev;
}

static int compare_fd(void *key1, void *key2)
{
   int fd1 = pointer_to_intptr(key1);
   int fd2 = pointer_to_intptr(key2);
   struct stat stat1, stat2;
   fstat(fd1, &stat1);
   fstat(fd2, &stat2);

   return stat1.st_dev != stat2.st_dev ||
         stat1.st_ino != stat2.st_ino ||
         stat1.st_rdev != stat2.st_rdev;
}

struct pipe_screen *
pipe_screen_reference(int fd)
{
   struct pipe_screen *pscreen;

   if (!fd_tab) {
      fd_tab = util_hash_table_create(hash_fd, compare_fd);
      return NULL;
   }

   pscreen = util_hash_table_get(fd_tab, intptr_to_pointer(fd));
   if (pscreen)
      pipe_reference(NULL, &pscreen->reference);

   return pscreen;
}

boolean
pipe_screen_unreference(struct pipe_screen *pscreen)
{
   boolean destroy;

   if (!pscreen)
      return FALSE;

   /* Work-around until all pipe_screens have ref counting */
   if (!pipe_is_referenced(&pscreen->reference)) {
      pscreen->destroy(pscreen);
      return TRUE;
   }

   destroy = pipe_reference(&pscreen->reference, NULL);
   if (destroy) {
      int fd = pscreen->fd;

      pscreen->destroy(pscreen);

      if (fd >= 0) {
         util_hash_table_remove(fd_tab, intptr_to_pointer(fd));
         close(fd);
      }
   }
   return destroy;
}


void pipe_screen_reference_init(struct pipe_screen *pscreen, int fd)
{
   pscreen->fd = fd;
   pipe_reference_init(&pscreen->reference, 1);
   util_hash_table_set(fd_tab, intptr_to_pointer(fd), pscreen);
}
