#ifndef _LINUX_ERRNO_H
#define _LINUX_ERRNO_H

#include <uapi/linux/errno.h>


/*
 * These should never be seen by user programs.  To return one of ERESTART*
 * codes, signal_pending() MUST be set.  Note that ptrace can observe these
 * at syscall exit tracing, but they will never be left for the debugged user
 * process to see.
 */
#define ERESTARTSYS	512
#define ERESTARTNOINTR	513
#define ERESTARTNOHAND	514	/* restart if no handler.. */
#define ENOIOCTLCMD	515	/* No ioctl command */
#define ERESTART_RESTARTBLOCK 516 /* restart by calling sys_restart_syscall */
#define EPROBE_DEFER	517	/* Driver requests probe retry */
#define EOPENSTALE	518	/* open found a stale dentry */

/* Defined for the NFSv3 protocol */
#define EBADHANDLE	521	/* Illegal NFS file handle */
#define ENOTSYNC	522	/* Update synchronization mismatch */
#define EBADCOOKIE	523	/* Cookie is stale */
#define ENOTSUPP	524	/* Operation is not supported */
#define ETOOSMALL	525	/* Buffer or request is too small */
#define ESERVERFAULT	526	/* An untranslatable error occurred */
#define EBADTYPE	527	/* Type not supported by server */
#define EJUKEBOX	528	/* Request initiated, but will not complete before timeout */
#define EIOCBQUEUED	529	/* iocb queued, will get completion event */

#define SAMSUNG_HYPERUART_PREALLOC_SUPPORT	/* pre-allocated memory */
#define SAMSUNG_USBNET_HYPERUART_IFNAME		/* interface name change */
#define SAMSUNG_HYPERUART_SDB_PUSH_FIX		/* Reduce Rx buffer size */

#define SAMSUNG_PATCH_WITH_USB_HOTPLUG          // patch for usb hotplug
#define SAMSUNG_PATCH_WITH_USB_HOTPLUG_MREADER  // patch for usb multicard reader
#define SAMSUNG_PATCH_WITH_USB_ENHANCEMENT      // stable patch for enhanced speed  and compatibility
#define SAMSUNG_PATCH_WITH_USB_HID_DISCONNECT_BUGFIX                    // patch fixes hid disconnect issues at suspend and manual disconnect time
#define KKS_DEBUG(f,a...)
#if defined(CONFIG_SAMSUNG_USB_PARALLEL_RESUME)||defined(CONFIG_SAMSUNG_USB_PARALLEL_RESUME_MODULE)
#define PARALLEL_RESET_RESUME_USER_PORT_DEVICES                //Patch to enable the reset resume of usb devices connected on user port.
#endif
#define SAMSUNG_USB_BTWIFI_RESET_WAIT                        //Patch to wait in a loop till WiFi, BTHUB or hub 1-1 ready for port-reset
#if defined(CONFIG_USB_AMBIENTMODE)				//Ambient Mode CONFIG selected
#define USB_AMBIENTMODE						//Ambient Mode support
#endif
#if defined(CONFIG_ARCH_SDP1406)  //HawkM only
#define SAMSUNG_PATCH_TASK_AFFINITY_FOR_PREVENT_OHCI_HANG
#endif
#define SAMSUNG_PATCH_RMB_WMB_AT_UNLINK                                   //Patch to add rmb and wmb at unlinking of urb
#if defined(CONFIG_ARCH_SDP1406) || defined(CONFIG_ARCH_SDP1404)
#define SAMSUNG_USB_FULL_SPEED_BT_MODIFY_GIVEBACK_URB            //Patch to divide some of operations of hcd_giveback_urb to another function and put that function under a lock.
#endif

#endif
