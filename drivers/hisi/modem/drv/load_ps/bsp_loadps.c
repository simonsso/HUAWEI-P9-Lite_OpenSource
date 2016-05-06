/*lint -save -e534 -e586 -e578 -e801 -e737 -e713 -e830*/
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/kthread.h>
#include <bsp_loadps.h>
#include <bsp_icc.h>
#include <drv_comm.h>
#include <mdrv_sysboot.h>
#include <loadps.h>

#include "osl_types.h"
#include "osl_sem.h"
#include <bsp_slice.h>
#include "../loadm/load_image.h"

/*loadps����Ҫ���ݽṹ*/
struct bsp_loadps_main_stru g_loadpsMain = {0, };

/*****************************************************************************
 ��������  : ��ʾloadpsģ��ĵ�����Ϣ
*****************************************************************************/
static ssize_t loadps_debug_info_get(struct device *dev, struct device_attribute *attr, char *buf)
{
    int x = 0 ;
    /* coverity[secure_coding] */
    x += sprintf(buf + x, "ps load rat_mode 0x%x\n", g_loadpsMain.loadinfo_debug.ps_loadinfo_rat_mode);
    x += sprintf(buf + x, "ps load phy_loadaddress 0x%x\n", g_loadpsMain.loadinfo_debug.ps_loadinfo_phy_loadaddress);
    x += sprintf(buf + x, "ps load total_size 0x%x\n", g_loadpsMain.loadinfo_debug.ps_loadinfo_total_size);
    x += sprintf(buf + x, "ps load result 0x%x\n",     g_loadpsMain.loadinfo_debug.ps_loadinfo_result);
    x += sprintf(buf + x, "ps_callback_ms 0x%x\n", g_loadpsMain.timestamp_debug.ps_callback_ms);
    x += sprintf(buf + x, "ps_task_wakeup_ms 0x%x\n", g_loadpsMain.timestamp_debug.ps_task_wakeup_ms);
    x += sprintf(buf + x, "ps_load_image_success_ms 0x%x\n", g_loadpsMain.timestamp_debug.ps_load_image_success_ms);
    x += sprintf(buf + x, "ps_load_image_total_elapse_ms 0x%x\n", g_loadpsMain.timestamp_debug.ps_load_image_total_elapse_ms);
    return x;
}

static DEVICE_ATTR(loadps_debug_info, S_IRUSR ,
                                    loadps_debug_info_get,
                                    NULL);


static struct attribute *load_ps_attributes[] = {
        &dev_attr_loadps_debug_info.attr,
        NULL
};

static const struct attribute_group load_ps_group = {
    .attrs = load_ps_attributes,
};

/*****************************************************************************
 ��������  : ��̬�޸�loadpsģ��ĵ��Լ���
 �������  : Ҫ�޸ĵļ����ַ���
 �������  : ��
 �� �� ֵ  : �ɹ�����OK,ʧ�ܷ���ERROR
*****************************************************************************/
s32 bsp_load_ps_callback ( u32 channel_id , u32 len, void* context )
{
    s32 ret =0;
    /*��¼һ��icc�ص�ʱ��ʱ��*/
    g_loadpsMain.timestamp_debug.ps_callback_ms = bsp_get_elapse_ms();
    /*������飬�������Ƿ���ȷ*/
    if((len == 0) || (len != sizeof(loadps_msg)))
	{
		loadps_trace(BSP_LOG_LEVEL_ERROR ,"len = 0x%x, sizeof(loadps_msg) = 0x%x\n", len, sizeof(loadps_msg));
        return -1;
	}
    /*������飬��channel_id�Ƿ�ƥ��*/
    if( channel_id != LOADPS_ICC_IFC_ID )
    {
        loadps_trace(BSP_LOG_LEVEL_ERROR ,"channel_id = 0x%x, LOADPS_ICC_IFC_ID = 0x%x\n", channel_id, LOADPS_ICC_IFC_ID);
        return -1;
    }
    /*��ʼ��C�˸�A���͵�ICC����*/
    ret = bsp_icc_read(channel_id, (u8*)(&g_loadpsMain.loadps_msg), sizeof(loadps_msg));
    /*�������ݲ���ȷ����û������*/
    if(((u32)ret != sizeof(loadps_msg)) || (ret <= 0))
    {
        loadps_trace(BSP_LOG_LEVEL_ERROR ,"! <%s> icc_read %d.\n", __FUNCTION__, ret);
        return -1;
    }
    loadps_trace(BSP_LOG_LEVEL_INFO ," bsp_load_ps_callback wakeup loadps task\n");
    /*����loadps������ִ�м���*/
    osl_sem_up(&g_loadpsMain.task_mutex);

    return 0;
}


/*****************************************************************************
 �� �� ��  : load_ps_image
 ��������  : �����ļ��ļ���
 �������  : loadps_msg
 �������  : ��
*****************************************************************************/
int load_ps_image(loadps_msg * loadps_msg)
{
	int ret = 0;
	enum SVC_SECBOOT_IMG_TYPE type;

    if( loadps_msg == NULL)
    {
        loadps_trace(BSP_LOG_LEVEL_ERROR ,"load_ps_image loadps_msg error.\n");
        return -1;
    }
    switch ( loadps_msg->rat_mode )
    {
        case DRV_RAT_MODE_UMTS_FDD:
            type = WAS;
            break;
        case DRV_RAT_MODE_UMTS_TDD:
            type = TAS;
            break;
        default:
            loadps_trace(BSP_LOG_LEVEL_ERROR ,"rat_mode error %x.\n",loadps_msg->rat_mode);
            return -1;
    }
	ret = bsp_load_modem_single_image(type, loadps_msg->addr, loadps_msg->size);
	if (ret)
    {
        loadps_trace(BSP_LOG_LEVEL_ERROR ,"load image type %d failed, ret %d\n", type, ret);
        return ret;
    }

    return ret;
}


/*****************************************************************************
 �� �� ��  : load_ps_task
 ��������  : ������ص�����
 �������  : void* obj
 �������  : ��
*****************************************************************************/
s32 load_ps_task(void* obj)
{
    s32 ret;
    u32 channel_id;
    loadps_status  status_msg;
    u32 msg_size =0;
    u32 elapsed;

    loadps_trace(BSP_LOG_LEVEL_ERROR ," <%s> entry.\n", __FUNCTION__);

    while(1)
    {
        loadps_trace(BSP_LOG_LEVEL_INFO ," start take g_loadpsMain.task_mutex .\n");
        /* coverity[check_return] */
        osl_sem_down(&g_loadpsMain.task_mutex);
        g_loadpsMain.timestamp_debug.ps_task_wakeup_ms = bsp_get_elapse_ms();
        elapsed = g_loadpsMain.timestamp_debug.ps_task_wakeup_ms - g_loadpsMain.timestamp_debug.ps_callback_ms;

        loadps_trace(BSP_LOG_LEVEL_ERROR ," success take g_loadpsMain.task_mutex elapsed wakeuptime %dms time.\n", elapsed);
        memset( &status_msg,0x00,sizeof (status_msg));
        channel_id = LOADPS_ICC_IFC_ID;
        /*δ��ʼ����ɻ��ߴ���˯��״̬������icc������������*/
        if(g_loadpsMain.eInitFlag != EN_LOADPS_INIT_FINISH)
        {
            continue;
        }

        g_loadpsMain.opState = EN_LOADPS_DOING;
        wake_lock(&g_loadpsMain.wake_lock);

        loadps_trace(BSP_LOG_LEVEL_INFO ,"icc read addr 0x%x size 0x%0x.\n",g_loadpsMain.loadps_msg.addr,g_loadpsMain.loadps_msg.size);
        loadps_trace(BSP_LOG_LEVEL_INFO ," start load image to sram .\n");
        g_loadpsMain.loadinfo_debug.ps_loadinfo_rat_mode = g_loadpsMain.loadps_msg.rat_mode;
        g_loadpsMain.loadinfo_debug.ps_loadinfo_phy_loadaddress = g_loadpsMain.loadps_msg.addr;
        g_loadpsMain.loadinfo_debug.ps_loadinfo_total_size  = g_loadpsMain.loadps_msg.size;
        /*ִ��˽���ļ����ع���*/
        status_msg.result = load_ps_image(&g_loadpsMain.loadps_msg);
        g_loadpsMain.loadinfo_debug.ps_loadinfo_result = status_msg.result;
        g_loadpsMain.timestamp_debug.ps_load_image_success_ms = bsp_get_elapse_ms();
        g_loadpsMain.timestamp_debug.ps_load_image_total_elapse_ms = g_loadpsMain.timestamp_debug.ps_load_image_success_ms - g_loadpsMain.timestamp_debug.ps_callback_ms;

        loadps_trace(BSP_LOG_LEVEL_ERROR ," complete load image to sram result %x total load_ps_image time %d ms.\n",status_msg.result,g_loadpsMain.timestamp_debug.ps_load_image_total_elapse_ms);

        /*A�����ڸ�C�˷��ͼ������ݵĽ��*/
        msg_size = sizeof(status_msg);
        ret = bsp_icc_send( ICC_CPU_MODEM, channel_id, (u8*)(&status_msg), msg_size);
        /*�ж�һ�������Ƿ��ͳɹ���*/
        if ( msg_size != ret )
        {
            loadps_trace(BSP_LOG_LEVEL_ERROR ,"ret = 0x%x, msg_size = 0x%x\n", ret, msg_size);
        }
        /*�˹���ִ�����˺���������һ��*/
        memset(&g_loadpsMain.loadps_msg,0x00,sizeof (loadps_msg));
        g_loadpsMain.opState = EN_LOADPS_DONE;
        wake_unlock(&g_loadpsMain.wake_lock);
        loadps_trace(BSP_LOG_LEVEL_INFO ,"load image complete.\n");
    }
}


/*****************************************************************************
 ��������  : loadpsģ���ʼ��
 �������  : ��
 �� �� ֵ  : void
*****************************************************************************/
static int __init his_loadps_probe(struct platform_device *pdev)
{
    s32 ret = 0;
    struct sched_param sch_para;

    /*��̬���ص�ʵʱ��Ҫ��ܸ�*/
    sch_para.sched_priority = 15;
    loadps_trace(BSP_LOG_LEVEL_ERROR, " his_loadps_probe.\n");
    memset(&g_loadpsMain,0x00,sizeof (struct bsp_loadps_main_stru));
    osl_sem_init(0, &(g_loadpsMain.task_mutex));

    wake_lock_init(&g_loadpsMain.wake_lock,WAKE_LOCK_SUSPEND, "loadps_wakelock");
    /*�����ں��߳�*/
    g_loadpsMain.taskid = kthread_run(load_ps_task, BSP_NULL, "loadps");
    if (IS_ERR(g_loadpsMain.taskid))
    {
        loadps_trace(BSP_LOG_LEVEL_ERROR, " <%s> kthread_run failed.\n", __FUNCTION__);
        return BSP_ERROR;
    }
    /*�����ں��̵߳����ȼ�*/
    if (BSP_OK != sched_setscheduler(g_loadpsMain.taskid, SCHED_FIFO, &sch_para))
    {
        loadps_trace(BSP_LOG_LEVEL_ERROR, " <%s> sched_setscheduler failed.\n", __FUNCTION__);
        return BSP_ERROR;
    }

    g_loadpsMain.eInitFlag = EN_LOADPS_INIT_FINISH;

    ret = bsp_icc_event_register(LOADPS_ICC_IFC_ID, bsp_load_ps_callback, NULL, NULL, NULL);
    if(ret)
    {
        loadps_trace(BSP_LOG_LEVEL_ERROR, " <%s> bsp_icc_event_register failed.\n", __FUNCTION__);
        return BSP_ERROR;
    }
    ret = sysfs_create_group(&pdev->dev.kobj, &load_ps_group);
    if (0 != ret)
    {
        loadps_trace(BSP_LOG_LEVEL_ERROR, " sysfs_create_group failed.\n");
        return -ENXIO;
    }
    loadps_trace(BSP_LOG_LEVEL_ERROR, " <%s> success.\n", __FUNCTION__);

    return BSP_OK;
}


/*****************************************************************************
 ��������  : loadpsģ��ж�ؽӿں���
 �������  : ��
 �� �� ֵ  : void
*****************************************************************************/
static int his_loadps_remove(struct platform_device *dev)
{
    s32 ret;

    loadps_trace(BSP_LOG_LEVEL_ERROR, " <%s> entry.\n", __FUNCTION__);

    g_loadpsMain.eInitFlag = EN_LOADPS_INIT_INVALID;

    osl_sema_delete(&g_loadpsMain.task_mutex);

    kthread_stop(g_loadpsMain.taskid);
    sysfs_remove_group(&dev->dev.kobj, &load_ps_group);
    ret = bsp_icc_event_unregister(LOADPS_ICC_IFC_ID);
    if(ret)
    {
        loadps_trace(BSP_LOG_LEVEL_ERROR, " <%s> bsp_icc_event_unregister failed.\n", __FUNCTION__);
        return BSP_ERROR;
    }
    return BSP_OK;
}



static struct platform_device his_loadps_device =
{
    .name = "his_loadps",
    .id = 0,
    .dev = {
    .init_name = "his_loadps",
    },
};

static struct platform_driver his_loadps_drv = {
    .probe      = his_loadps_probe,
    .remove     = his_loadps_remove,
    .driver     = {
        .name     = "his_loadps",
        .owner    = THIS_MODULE,
    },
};

static int __init his_loadps_init_driver(void)
{
    int ret = 0;
    printk(KERN_ERR "loadps: ");
    /*����һ��loadpsģ��Ĵ�ӡ������*/
    bsp_mod_level_set(BSP_MODU_LOADPS,BSP_LOADPS_LOG_LEVEL);
    ret = platform_device_register(&his_loadps_device);
    if(ret)
    {
        printk(KERN_ERR "register his_loadps device failed\n");
        return ret;
    }

    ret = platform_driver_register(&his_loadps_drv);
    if(ret)
    {
        printk(KERN_ERR "register his_loadps driver failed\n");
        platform_device_unregister(&his_loadps_device);
    }

    return ret;

}

static void __exit his_modem_exit_driver(void)
{
    platform_driver_unregister(&his_loadps_drv);
    platform_device_unregister(&his_loadps_device);
}
module_init(his_loadps_init_driver);
module_exit(his_modem_exit_driver);