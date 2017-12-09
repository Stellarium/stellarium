/*

  Copyright (c) 2005 Finger Lakes Instrumentation (FLI), L.L.C.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

        Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

        Redistributions in binary form must reproduce the above
        copyright notice, this list of conditions and the following
        disclaimer in the documentation and/or other materials
        provided with the distribution.

        Neither the name of Finger Lakes Instrumentation (FLI), L.L.C.
        nor the names of its contributors may be used to endorse or
        promote products derived from this software without specific
        prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

  ======================================================================

  Finger Lakes Instrumentation, L.L.C. (FLI)
  web: http://www.fli-cam.com
  email: support@fli-cam.com

*/

#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kref.h>
#include <linux/errno.h>
#include <linux/usb.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#ifdef SGREAD
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/scatterlist.h>
//#include <asm/scatterlist.h>
#endif

#include "fliusb.h"
#include "fliusb_ioctl.h"

MODULE_AUTHOR("Finger Lakes Instrumentation, L.L.C. <support@flicamera.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION("1.3");

/* initMUTEX was removed in 2.6.37, I hate Linux, minor revision changes
 * should include a warning when this shit is going to happen with
 * suggested alternatives! */

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36) && !defined(init_MUTEX)
#define init_MUTEX(sem) sema_init(sem, 1)
#endif

struct mutex fliusb_mutex;

/* Module parameters */
typedef struct
{
    unsigned int buffersize;
    unsigned int timeout;
} fliusb_param_t;

static fliusb_param_t defaults = {
    .buffersize = FLIUSB_BUFFERSIZE,
    .timeout    = FLIUSB_TIMEOUT,
};

#define FLIUSB_MOD_PARAMETERS                                           \
    FLIUSB_MOD_PARAM(buffersize, uint, "USB bulk transfer buffer size") \
    FLIUSB_MOD_PARAM(timeout, uint, "USB bulk transfer timeout (msec)")

#define FLIUSB_MOD_PARAM(var, type, desc)                 \
    module_param_named(var, defaults.var, type, S_IRUGO); \
    MODULE_PARM_DESC(var, desc);

FLIUSB_MOD_PARAMETERS;

#undef FLIUSB_MOD_PARAM

/* Devices supported by this driver */
static struct usb_device_id fliusb_table[] = {

#define FLIUSB_PROD(name, prodid) { USB_DEVICE(FLIUSB_VENDORID, prodid) },

    FLIUSB_PRODUCTS

#undef FLIUSB_PROD

    {}, /* Terminating entry */
};

MODULE_DEVICE_TABLE(usb, fliusb_table);

/* Forward declarations of file operation functions */
static int fliusb_open(struct inode *inode, struct file *file);
static int fliusb_release(struct inode *inode, struct file *file);
static ssize_t fliusb_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos);
static ssize_t fliusb_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *ppos);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36)
static long fliusb_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
#else
static int fliusb_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
#endif

static struct file_operations fliusb_fops = {
    .owner = THIS_MODULE,
    .read  = fliusb_read,
    .write = fliusb_write,
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36)
    .unlocked_ioctl = fliusb_ioctl,
#else
    .ioctl = fliusb_ioctl,
#endif
    .open    = fliusb_open,
    .release = fliusb_release,
};

static struct usb_class_driver fliusb_class = {
    .name = "usb/" FLIUSB_NAME "%d",
    .fops = &fliusb_fops,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14))
    .mode = S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
#endif
    .minor_base = FLIUSB_MINOR_BASE,
};

/* Forward declarations of USB driver functions */
static int fliusb_probe(struct usb_interface *interface, const struct usb_device_id *id);
static void fliusb_disconnect(struct usb_interface *interface);

static struct usb_driver fliusb_driver = {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14))
    .owner = THIS_MODULE,
#endif
    .name       = FLIUSB_NAME,
    .probe      = fliusb_probe,
    .disconnect = fliusb_disconnect,
    .id_table   = fliusb_table,
};

static void fliusb_delete(struct kref *kref)
{
    fliusb_t *dev;

    dev = container_of(kref, fliusb_t, kref);
    usb_put_dev(dev->usbdev);
    if (dev->buffer)
        kfree(dev->buffer);
    kfree(dev);

    return;
}

static int fliusb_allocbuffer(fliusb_t *dev, unsigned int size)
{
    void *tmp;
    int err = 0;

    if (size == 0)
        return -EINVAL;

    if (down_interruptible(&dev->buffsem))
        return -ERESTARTSYS;

    if ((tmp = kmalloc(size, GFP_KERNEL)) == NULL)
    {
        FLIUSB_WARN("kmalloc() failed: could not allocate %d byte buffer", size);
        err = -ENOMEM;
        goto done;
    }

    if (dev->buffer != NULL)
        kfree(dev->buffer);
    dev->buffer     = tmp;
    dev->buffersize = size;

done:

    up(&dev->buffsem);

    return err;
}

static int fliusb_open(struct inode *inode, struct file *file)
{
    fliusb_t *dev;
    struct usb_interface *interface;
    int minor;

    minor = iminor(inode);

    if ((interface = usb_find_interface(&fliusb_driver, minor)) == NULL)
    {
        FLIUSB_ERR("no interface found for minor number %d", minor);
        return -ENODEV;
    }

    if ((dev = usb_get_intfdata(interface)) == NULL)
    {
        FLIUSB_WARN("no device data for minor number %d", minor);
        return -ENODEV;
    }

    /* increment usage count */
    kref_get(&dev->kref);

    /* save a pointer to the device structure for later use */
    file->private_data = dev;

    return 0;
}

static int fliusb_release(struct inode *inode, struct file *file)
{
    fliusb_t *dev;

    if ((dev = file->private_data) == NULL)
    {
        FLIUSB_ERR("no device data for minor number %d", iminor(inode));
        return -ENODEV;
    }

    /* decrement usage count */
    kref_put(&dev->kref, fliusb_delete);

    return 0;
}

static int fliusb_simple_bulk_read(fliusb_t *dev, unsigned int pipe, char __user *userbuffer, size_t count,
                                   unsigned int timeout)
{
    int err, cnt;

    if (count > dev->buffersize)
        count = dev->buffersize;

    if (!access_ok(VERIFY_WRITE, userbuffer, count))
        return -EFAULT;

    if (down_interruptible(&dev->buffsem))
        return -ERESTARTSYS;

    /* a simple blocking bulk read */
    if ((err = usb_bulk_msg(dev->usbdev, pipe, dev->buffer, count, &cnt, timeout)))
    {
        cnt = err;
        goto done;
    }

    if (__copy_to_user(userbuffer, dev->buffer, cnt))
        cnt = -EFAULT;

done:

    up(&dev->buffsem);

    return cnt;
}

#ifdef SGREAD

static void fliusb_sg_bulk_read_timeout(unsigned long data)
{
    fliusb_t *dev = (fliusb_t *)data;

    FLIUSB_ERR("bulk read timed out");
    usb_sg_cancel(&dev->usbsg.sgreq);

    return;
}

static int fliusb_sg_bulk_read(fliusb_t *dev, unsigned int pipe, char __user *userbuffer, size_t count,
                               unsigned int timeout)
{
    int err, i;
    unsigned int numpg;
    size_t pgoffset;

    /* userbuffer must be aligned to a multiple of the endpoint's
     maximum packet size
  */
    if ((size_t)userbuffer % usb_maxpacket(dev->usbdev, pipe, 0))
    {
        FLIUSB_ERR("user buffer is not properly aligned: 0x%p %% 0x%04x", userbuffer,
                   usb_maxpacket(dev->usbdev, pipe, 0));
        return -EINVAL;
    }

    pgoffset = (size_t)userbuffer & (PAGE_SIZE - 1);
    numpg    = ((count + pgoffset - 1) >> PAGE_SHIFT) + 1;
    if (numpg > dev->usbsg.maxpg)
    {
        numpg = dev->usbsg.maxpg;
        count = PAGE_SIZE - pgoffset + PAGE_SIZE * (dev->usbsg.maxpg - 1);
    }

    if (down_interruptible(&dev->usbsg.sem))
        return -ERESTARTSYS;

    down_read(&current->mm->mmap_sem);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
    numpg = get_user_pages(numpg, 1, 0, dev->usbsg.userpg, NULL);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4, 6, 0)
    numpg = get_user_pages_remote(current, current->mm, (size_t)userbuffer & PAGE_MASK, numpg, 1, 0, dev->usbsg.userpg,
                                  NULL);
#else
    numpg = get_user_pages(current, current->mm, (size_t)userbuffer & PAGE_MASK, numpg, 1, 0, dev->usbsg.userpg, NULL);
#endif
    up_read(&current->mm->mmap_sem);

    if (numpg <= 0)
    {
        FLIUSB_ERR("get_user_pages() failed: %d", numpg);
        err = numpg;
        goto done;
    }

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24))
    dev->usbsg.slist[0].page   = dev->usbsg.userpg[0];
    dev->usbsg.slist[0].offset = pgoffset;
    dev->usbsg.slist[0].length = min(count, (size_t)(PAGE_SIZE - pgoffset));
#else
    sg_set_page(&dev->usbsg.slist[0], dev->usbsg.userpg[0], min(count, (size_t)(PAGE_SIZE - pgoffset)), pgoffset);
#endif

    if (numpg > 1)
    {
        for (i = 1; i < numpg - 1; i++)
        {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24))
            dev->usbsg.slist[i].page   = dev->usbsg.userpg[i];
            dev->usbsg.slist[i].offset = 0;
            dev->usbsg.slist[i].length = PAGE_SIZE;
        }

        dev->usbsg.slist[i].page   = dev->usbsg.userpg[i];
        dev->usbsg.slist[i].offset = 0;
        dev->usbsg.slist[i].length = ((size_t)userbuffer + count) & (PAGE_SIZE - 1);
        if (dev->usbsg.slist[i].length == 0)
            dev->usbsg.slist[i].length = PAGE_SIZE;
#else
            sg_set_page(&dev->usbsg.slist[i], dev->usbsg.userpg[i], PAGE_SIZE, 0);
        }

        if ((((size_t)userbuffer + count) & (PAGE_SIZE - 1)) == 0)
            sg_set_page(&dev->usbsg.slist[i], dev->usbsg.userpg[i], PAGE_SIZE, 0);
        else
            sg_set_page(&dev->usbsg.slist[i], dev->usbsg.userpg[i], ((size_t)userbuffer + count) & (PAGE_SIZE - 1), 0);

#endif
    }

    if ((err = usb_sg_init(&dev->usbsg.sgreq, dev->usbdev, pipe, 0, dev->usbsg.slist, numpg, 0, GFP_KERNEL)))
    {
        FLIUSB_ERR("usb_sg_init() failed: %d", err);
        goto done;
    }

    dev->usbsg.timer.expires  = jiffies + (timeout * HZ + 500) / 1000;
    dev->usbsg.timer.data     = (unsigned long)dev;
    dev->usbsg.timer.function = fliusb_sg_bulk_read_timeout;
    add_timer(&dev->usbsg.timer);

    /* wait for the transfer to complete */
    usb_sg_wait(&dev->usbsg.sgreq);

    del_timer_sync(&dev->usbsg.timer);

    if (dev->usbsg.sgreq.status)
    {
        FLIUSB_ERR("bulk read error %d; transfered %d bytes", (int)dev->usbsg.sgreq.status,
                   (int)dev->usbsg.sgreq.bytes);
        err = dev->usbsg.sgreq.status;
        goto done;
    }

    err = dev->usbsg.sgreq.bytes;

done:

    up(&dev->usbsg.sem);

    for (i = 0; i < numpg; i++)
    {
        if (!PageReserved(dev->usbsg.userpg[i]))
            SetPageDirty(dev->usbsg.userpg[i]);
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 6, 0)
        put_page(dev->usbsg.userpg[i]);
#else
        page_cache_release(dev->usbsg.userpg[i]);
#endif
    }

    return err;
}

#endif /* SGREAD */

static int fliusb_bulk_read(fliusb_t *dev, unsigned int pipe, char __user *userbuffer, size_t count,
                            unsigned int timeout)
{
    FLIUSB_DBG("pipe: 0x%08x; userbuffer: %p; count: %u; timeout: %u", pipe, userbuffer, count, timeout);

#ifdef SGREAD
    if (count > dev->buffersize)
        return fliusb_sg_bulk_read(dev, pipe, userbuffer, count, timeout);
    else
#endif /* SGREAD */
        return fliusb_simple_bulk_read(dev, pipe, userbuffer, count, timeout);
}

#ifndef ASYNCWRITE

static int fliusb_bulk_write(fliusb_t *dev, unsigned int pipe, const char __user *userbuffer, size_t count,
                             unsigned int timeout)
{
    int err, cnt;

    FLIUSB_DBG("pipe: 0x%08x; userbuffer: %p; count: %u; timeout: %u", pipe, userbuffer, count, timeout);

    if (count > dev->buffersize)
        count = dev->buffersize;

    if (down_interruptible(&dev->buffsem))
        return -ERESTARTSYS;

    if (copy_from_user(dev->buffer, userbuffer, count))
    {
        cnt = -EFAULT;
        goto done;
    }

    /* a simple blocking bulk write */
    if ((err = usb_bulk_msg(dev->usbdev, pipe, dev->buffer, count, &cnt, timeout)))
    {
        cnt = err;
        // reset USB in case of an error
        err = usb_reset_configuration(dev->usbdev);
        FLIUSB_DBG("configuration return: %d", err);
    }

done:

    up(&dev->buffsem);

    return cnt;
}

#else

static void fliusb_bulk_write_callback(struct urb *urb, struct pt_regs *regs)
{
    switch (urb->status)
    {
        case 0:
        case -ECONNRESET:
        case -ENOENT:
        case -ESHUTDOWN:
            break; /* These aren't noteworthy */

        default:
            FLIUSB_WARN("bulk write error %d; transfered %d bytes", urb->status, urb->actual_length);
            break;
    }

    usb_buffer_free(urb->dev, urb->transfer_buffer_length, urb->transfer_buffer, urb->transfer_dma);

    return;
}

static int fliusb_bulk_write(fliusb_t *dev, unsigned int pipe, const char __user *userbuffer, size_t count,
                             unsigned int timeout)
{
    int err;
    struct urb *urb = NULL;
    char *buffer    = NULL;

    FLIUSB_DBG("pipe: 0x%08x; userbuffer: %p; count: %u; timeout: %u", pipe, userbuffer, count, timeout);

    if (!access_ok(VERIFY_READ, userbuffer, count))
        return -EFAULT;

    if ((urb = usb_alloc_urb(0, GFP_KERNEL)) == NULL)
        return -ENOMEM;

    if ((buffer = usb_buffer_alloc(dev->usbdev, count, GFP_KERNEL, &urb->transfer_dma)) == NULL)
    {
        err = -ENOMEM;
        goto error;
    }

    if (__copy_from_user(buffer, userbuffer, count))
    {
        err = -EFAULT;
        goto error;
    }

    usb_fill_bulk_urb(urb, dev->usbdev, pipe, buffer, count, fliusb_bulk_write_callback, dev);
    urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

    if ((err = usb_submit_urb(urb, GFP_KERNEL)))
        goto error;

    usb_free_urb(urb);

    return count;

error:

    if (buffer)
        usb_buffer_free(dev->usbdev, count, buffer, urb->transfer_dma);
    if (urb)
        usb_free_urb(urb);

    return err;
}

#endif /* ASYNCWRITE */

static ssize_t fliusb_read(struct file *file, char __user *userbuffer, size_t count, loff_t *ppos)
{
    fliusb_t *dev;

    if (count == 0)
        return 0;

    if (*ppos)
        return -ESPIPE;

    dev = (fliusb_t *)file->private_data;

    return fliusb_bulk_read(dev, dev->rdbulkpipe, userbuffer, count, dev->timeout);
}

static ssize_t fliusb_write(struct file *file, const char __user *userbuffer, size_t count, loff_t *ppos)
{
    fliusb_t *dev;

    if (count == 0)
        return 0;

    if (*ppos)
        return -ESPIPE;

    dev = (fliusb_t *)file->private_data;

    return fliusb_bulk_write(dev, dev->wrbulkpipe, userbuffer, count, dev->timeout);
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36)
static long fliusb_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#else
static int fliusb_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
    fliusb_t *dev;
    union {
        u_int8_t uint8;
        unsigned int uint;
        fliusb_bulktransfer_t bulkxfer;
        fliusb_string_descriptor_t strdesc;
    } tmp;
    unsigned int tmppipe;
    int err;

    FLIUSB_DBG("cmd: %p; arg: %p", (void *)cmd, (void *)arg);

    if (_IOC_TYPE(cmd) != FLIUSB_IOC_TYPE || _IOC_NR(cmd) > FLIUSB_IOC_MAX)
        return -ENOTTY;

    /* Check that arg can be read from */
    if ((_IOC_DIR(cmd) & _IOC_WRITE) && !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd)))
        return -EFAULT;

    /* Check that arg can be written to */
    if ((_IOC_DIR(cmd) & _IOC_READ) && !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd)))
        return -EFAULT;

    dev = (fliusb_t *)file->private_data;

    switch (cmd)
    {
#define FLIUSB_IOC_GETCMD(cmd, val, type)           \
    case cmd:                                       \
        return __put_user(val, (type __user *)arg); \
        break;

        FLIUSB_IOC_GETCMD(FLIUSB_GETBUFFERSIZE, dev->buffersize, unsigned int);
        FLIUSB_IOC_GETCMD(FLIUSB_GETTIMEOUT, dev->timeout, unsigned int);
        FLIUSB_IOC_GETCMD(FLIUSB_GETRDEPADDR, (u_int8_t)(usb_pipeendpoint(dev->rdbulkpipe) | USB_DIR_IN), u_int8_t);
        FLIUSB_IOC_GETCMD(FLIUSB_GETWREPADDR, (u_int8_t)(usb_pipeendpoint(dev->wrbulkpipe) | USB_DIR_OUT), u_int8_t);

#undef FLIUSB_IOC_GETCMD

        case FLIUSB_SETRDEPADDR:
            if (__get_user(tmp.uint8, (u_int8_t __user *)arg))
                return -EFAULT;
            tmppipe = usb_rcvbulkpipe(dev->usbdev, tmp.uint8);
            if (usb_maxpacket(dev->usbdev, tmppipe, 0) == 0)
            {
                FLIUSB_ERR("invalid read USB bulk transfer endpoint address: 0x%02x", tmp.uint8);
                return -EINVAL;
            }
            dev->rdbulkpipe = tmppipe;
            return 0;
            break;

        case FLIUSB_SETWREPADDR:
            if (__get_user(tmp.uint8, (u_int8_t __user *)arg))
                return -EFAULT;
            tmppipe = usb_sndbulkpipe(dev->usbdev, tmp.uint8);
            if (usb_maxpacket(dev->usbdev, tmppipe, 1) == 0)
            {
                FLIUSB_ERR("invalid write USB bulk transfer endpoint address: 0x%02x", tmp.uint8);
                return -EINVAL;
            }
            dev->wrbulkpipe = tmppipe;
            return 0;
            break;

        case FLIUSB_SETBUFFERSIZE:
            if (__get_user(tmp.uint, (unsigned int __user *)arg))
                return -EFAULT;
            return fliusb_allocbuffer(dev, tmp.uint);
            break;

        case FLIUSB_SETTIMEOUT:
            if (__get_user(tmp.uint, (unsigned int __user *)arg))
                return -EFAULT;
            if (tmp.uint == 0)
            {
                FLIUSB_ERR("invalid timeout: %u", tmp.uint);
                return -EINVAL;
            }
            dev->timeout = tmp.uint;
            return 0;
            break;

        case FLIUSB_BULKREAD:
            if (__copy_from_user(&tmp.bulkxfer, (fliusb_bulktransfer_t __user *)arg, sizeof(fliusb_bulktransfer_t)))
                return -EFAULT;
            tmppipe = usb_rcvbulkpipe(dev->usbdev, tmp.bulkxfer.ep);
            return fliusb_bulk_read(dev, tmppipe, tmp.bulkxfer.buf, tmp.bulkxfer.count, tmp.bulkxfer.timeout);

        case FLIUSB_BULKWRITE:
            if (__copy_from_user(&tmp.bulkxfer, (fliusb_bulktransfer_t __user *)arg, sizeof(fliusb_bulktransfer_t)))
                return -EFAULT;
            tmppipe = usb_sndbulkpipe(dev->usbdev, tmp.bulkxfer.ep);
            return fliusb_bulk_write(dev, tmppipe, tmp.bulkxfer.buf, tmp.bulkxfer.count, tmp.bulkxfer.timeout);

        case FLIUSB_GET_DEVICE_DESCRIPTOR:
            if (__copy_to_user((void *)arg, &dev->usbdev->descriptor, sizeof(struct usb_device_descriptor)))
                return -EFAULT;
            return 0;
            break;

        case FLIUSB_GET_STRING_DESCRIPTOR:
            if (__copy_from_user(&tmp.strdesc, (fliusb_string_descriptor_t __user *)arg,
                                 sizeof(fliusb_string_descriptor_t)))
                return -EFAULT;

            memset(tmp.strdesc.buf, 0, sizeof(tmp.strdesc.buf));

            if ((err = usb_string(dev->usbdev, tmp.strdesc.index, tmp.strdesc.buf, sizeof(tmp.strdesc.buf))) < 0)
                FLIUSB_WARN("usb_string() failed: %d", err);

            if (__copy_to_user((void *)arg, &tmp.strdesc, sizeof(fliusb_string_descriptor_t)))
            {
                return -EFAULT;
            }

            return 0;
            break;

        default:
            FLIUSB_ERR("invalid ioctl request: %d", cmd);
            return -ENOTTY;
    }

    return -ENOTTY; /* shouldn't get here */
}

static int fliusb_initdev(fliusb_t **dev, struct usb_interface *interface, const struct usb_device_id *id)
{
    fliusb_t *tmpdev;
    int err;
    char prodstr[64] = "unknown";

    /* These vendor/product ID checks shouldn't be necessary */
    if (id->idVendor != FLIUSB_VENDORID)
    {
        FLIUSB_WARN("unexpectedly probing a non-FLI USB device");
        return -EINVAL;
    }

    switch (id->idProduct)
    {
#define FLIUSB_PROD(name, prodid) case name##_PRODID:
        FLIUSB_PRODUCTS
#undef FLIUSB_PROD
        break;

        default:
            FLIUSB_WARN("unsupported FLI USB device");
            return -EINVAL;
    }

    if ((tmpdev = kmalloc(sizeof(fliusb_t), GFP_KERNEL)) == NULL)
        return -ENOMEM;
    memset(tmpdev, 0, sizeof(*tmpdev));
    kref_init(&tmpdev->kref);

    tmpdev->usbdev    = usb_get_dev(interface_to_usbdev(interface));
    tmpdev->interface = interface;
    tmpdev->timeout   = defaults.timeout;

    switch (id->idProduct)
    {
        case FLIUSB_MAXCAM_PRODID:
        case FLIUSB_STEPPER_PRODID:
        case FLIUSB_FOCUSER_PRODID:
        case FLIUSB_FILTERWHEEL_PRODID:
            tmpdev->rdbulkpipe = usb_rcvbulkpipe(tmpdev->usbdev, FLIUSB_RDEPADDR);
            tmpdev->wrbulkpipe = usb_sndbulkpipe(tmpdev->usbdev, FLIUSB_WREPADDR);
            break;

        case FLIUSB_PROLINECAM_PRODID:
            tmpdev->rdbulkpipe = usb_rcvbulkpipe(tmpdev->usbdev, FLIUSB_PROLINE_RDEPADDR);
            tmpdev->wrbulkpipe = usb_sndbulkpipe(tmpdev->usbdev, FLIUSB_PROLINE_WREPADDR);
            break;

        default:
            FLIUSB_WARN("unsupported FLI USB device");
            err = -EINVAL;
            goto fail;
            break;
    }

    /* Check that the endpoints exist */
    if (usb_maxpacket(tmpdev->usbdev, tmpdev->rdbulkpipe, 0) == 0)
    {
        FLIUSB_ERR("invalid read USB bulk transfer endpoint address: 0x%02x",
                   usb_pipeendpoint(tmpdev->rdbulkpipe) | USB_DIR_IN);
        err = -ENXIO;
        goto fail;
    }
    if (usb_maxpacket(tmpdev->usbdev, tmpdev->wrbulkpipe, 1) == 0)
    {
        FLIUSB_ERR("invalid write USB bulk transfer endpoint address: 0x%02x",
                   usb_pipeendpoint(tmpdev->wrbulkpipe) | USB_DIR_OUT);
        err = -ENXIO;
        goto fail;
    }

    init_MUTEX(&tmpdev->buffsem);
    if ((err = fliusb_allocbuffer(tmpdev, defaults.buffersize)))
        goto fail;

#ifdef SGREAD
    tmpdev->usbsg.maxpg = NUMSGPAGE;
    init_timer(&tmpdev->usbsg.timer);
    init_MUTEX(&tmpdev->usbsg.sem);
#endif /* SGREAD */

    if ((err = usb_string(tmpdev->usbdev, tmpdev->usbdev->descriptor.iProduct, prodstr, sizeof(prodstr))) < 0)
        FLIUSB_WARN("usb_string() failed: %d", err);

    FLIUSB_INFO("FLI USB device found: '%s'", prodstr);

    *dev = tmpdev;

    return 0;

fail:

    kref_put(&tmpdev->kref, fliusb_delete);
    return err;
}

static int fliusb_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    fliusb_t *dev;
    int err;

    if ((err = fliusb_initdev(&dev, interface, id)))
        return err;

    /* save a pointer to the device structure in this interface */
    usb_set_intfdata(interface, dev);

    /* register the device */
    if ((err = usb_register_dev(interface, &fliusb_class)))
    {
        FLIUSB_ERR("usb_register_dev() failed: %d", err);
        usb_set_intfdata(interface, NULL);
        kref_put(&dev->kref, fliusb_delete);
        return err;
    }

    FLIUSB_INFO("FLI USB device attached; rdepaddr: 0x%02x; wrepaddr: 0x%02x; "
                "buffersize: %d; timeout: %d",
                usb_pipeendpoint(dev->rdbulkpipe) | USB_DIR_IN, usb_pipeendpoint(dev->wrbulkpipe) | USB_DIR_OUT,
                dev->buffersize, dev->timeout);

    return 0;
}

static void fliusb_disconnect(struct usb_interface *interface)
{
    fliusb_t *dev;

    /* this is to block entry to fliusb_open() while the device is being
     disconnected
  */
    mutex_lock(&fliusb_mutex);

    dev = usb_get_intfdata(interface);
    usb_set_intfdata(interface, NULL);

    /* give back the minor number we were using */
    usb_deregister_dev(interface, &fliusb_class);

    mutex_unlock(&fliusb_mutex);

    /* decrement usage count */
    kref_put(&dev->kref, fliusb_delete);

    FLIUSB_INFO("FLI USB device disconnected");
}

static int __init fliusb_init(void)
{
    int err;

    if ((err = usb_register(&fliusb_driver)))
        FLIUSB_ERR("usb_register() failed: %d", err);

    mutex_init(&fliusb_mutex);

    FLIUSB_INFO(FLIUSB_NAME " module loaded");

    return err;
}

static void __exit fliusb_exit(void)
{
    usb_deregister(&fliusb_driver);

    FLIUSB_INFO(FLIUSB_NAME " module unloaded");

    return;
}

module_init(fliusb_init);
module_exit(fliusb_exit);
