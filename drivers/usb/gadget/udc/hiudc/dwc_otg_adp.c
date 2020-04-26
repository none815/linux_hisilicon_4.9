/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/dwc_otg_adp.c $
 * $Revision: #16 $
 * $Date: 2013/04/22 $
 * $Change: 2211149 $
 *
 * Synopsys HS OTG Linux Software Driver and documentation (hereinafter,
 * "Software") is an Unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * ========================================================================== */

#include "dwc_os.h"
#include "dwc_otg_regs.h"
#include "dwc_otg_cil.h"
#include "dwc_otg_adp.h"

/** @file
 *
 * This file contains the most of the Attach Detect Protocol implementation for
 * the driver to support OTG Rev2.0.
 *
 */

void dwc_otg_adp_write_reg(dwc_otg_core_if_t * core_if, uint32_t value)
{
	adpctl_data_t adpctl;

	adpctl.d32 = value;
	adpctl.b.ar = 0x2;

	DWC_WRITE_REG32(&core_if->core_global_regs->adpctl, adpctl.d32);

	while (adpctl.b.ar) {
		adpctl.d32 = DWC_READ_REG32(&core_if->core_global_regs->adpctl);
	}

}

/**
 * Function is called to read ADP registers
 */
uint32_t dwc_otg_adp_read_reg(dwc_otg_core_if_t * core_if)
{
	adpctl_data_t adpctl;

	adpctl.d32 = 0;
	adpctl.b.ar = 0x1;

	DWC_WRITE_REG32(&core_if->core_global_regs->adpctl, adpctl.d32);

	while (adpctl.b.ar) {
		adpctl.d32 = DWC_READ_REG32(&core_if->core_global_regs->adpctl);
	}

	return adpctl.d32;
}

/**
 * Function is called to read ADPCTL register and filter Write-clear bits
 */
uint32_t dwc_otg_adp_read_reg_filter(dwc_otg_core_if_t * core_if)
{
	adpctl_data_t adpctl;

	adpctl.d32 = dwc_otg_adp_read_reg(core_if);
	adpctl.b.adp_tmout_int = 0;
	adpctl.b.adp_prb_int = 0;
	adpctl.b.adp_tmout_int = 0;

	return adpctl.d32;
}

/**
 * Function is called to write ADP registers
 */
void dwc_otg_adp_modify_reg(dwc_otg_core_if_t * core_if, uint32_t clr,
			    uint32_t set)
{
	dwc_otg_adp_write_reg(core_if,
			      (dwc_otg_adp_read_reg(core_if) & (~clr)) | set);
}

/**
 * Start the ADP Initial Probe timer to detect if Port Connected interrupt is
 * not asserted within 1.1 seconds.
 *
 * @param core_if the pointer to core_if strucure.
 */
void dwc_otg_adp_vbuson_timer_start(dwc_otg_core_if_t * core_if)
{
	core_if->adp.vbuson_timer_started = 1;
	if (core_if->adp.vbuson_timer)
	{
		DWC_PRINTF("SCHEDULING VBUSON TIMER\n");
		/* 1.1 secs + 60ms necessary for cil_hcd_start*/
		DWC_TIMER_SCHEDULE(core_if->adp.vbuson_timer, 1160);
	} else {
		DWC_WARN("VBUSON_TIMER = %p\n",core_if->adp.vbuson_timer);
	}
}

/**
 * Starts the ADP Probing
 *
 * @param core_if the pointer to core_if structure.
 */
uint32_t dwc_otg_adp_probe_start(dwc_otg_core_if_t * core_if)
{

	adpctl_data_t adpctl = {.d32 = 0};
	gpwrdn_data_t gpwrdn;

	if (core_if->stop_adpprb) {
		core_if->stop_adpprb = 0;
		return 0;
	}

	dwc_otg_disable_global_interrupts(core_if);
	DWC_DEBUGPL(DBG_ANY, "ADP Probe Start\n");
	core_if->adp.probe_enabled = 1;

	adpctl.b.adpres = 1;
	dwc_otg_adp_write_reg(core_if, adpctl.d32);

	while (adpctl.b.adpres) {
		adpctl.d32 = dwc_otg_adp_read_reg(core_if);
	}

	adpctl.d32 = 0;
	gpwrdn.d32 = DWC_READ_REG32(&core_if->core_global_regs->gpwrdn);

	/* In Host mode unmask SRP detected interrupt also change the
	 * probe preiod accordingly */
	if (!gpwrdn.b.idsts) {
		gpwrdn.d32 = 0;
		gpwrdn.b.srp_det_msk = 1;
		adpctl.b.prb_per = 0;
	}
	else {
		gpwrdn.d32 = 0;
		gpwrdn.b.srp_det_msk = 1;
		DWC_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, gpwrdn.d32, 0);
		gpwrdn.d32 = 0;
		gpwrdn.b.sts_chngint_msk = 1;
		adpctl.b.prb_per = 1;
	}
	DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, 0, gpwrdn.d32);

	adpctl.b.adp_tmout_int_msk = 1;
	adpctl.b.adp_prb_int_msk = 1;
	adpctl.b.prb_dschg = 1;
	adpctl.b.prb_delta = 1;
	dwc_otg_adp_write_reg(core_if, adpctl.d32);

	adpctl.b.adpen = 1;
	adpctl.b.enaprb = 1;
	dwc_otg_adp_write_reg(core_if, adpctl.d32);
	DWC_DEBUGPL(DBG_ANY, "ADP Probe Finish\n");

	return 0;
}

/**
 * Starts the ADP Sense timer to detect if ADP Sense interrupt is not asserted
 * within 3 seconds.
 *
 * @param core_if the pointer to core_if strucure.
 */
void dwc_otg_adp_sense_timer_start(dwc_otg_core_if_t * core_if)
{
	core_if->adp.sense_timer_started = 1;
	DWC_TIMER_SCHEDULE(core_if->adp.sense_timer, 3300 /* 3.3 secs */ );
}

/**
 * Starts the ADP Sense
 *
 * @param core_if the pointer to core_if strucure.
 */
uint32_t dwc_otg_adp_sense_start(dwc_otg_core_if_t * core_if)
{
	adpctl_data_t adpctl;

	DWC_DEBUGPL(DBG_PCD, "ADP Sense Start\n");

	/* Set ADP reset bit*/
	adpctl.d32 = dwc_otg_adp_read_reg_filter(core_if);
	adpctl.b.adpres = 1;
	dwc_otg_adp_write_reg(core_if, adpctl.d32);

	while (adpctl.b.adpres) {
		adpctl.d32 = dwc_otg_adp_read_reg(core_if);
	}

	/* Unmask ADP sense interrupt and mask all other from the core */
	adpctl.d32 = dwc_otg_adp_read_reg_filter(core_if);
	adpctl.b.adp_sns_int_msk = 1;
	dwc_otg_adp_write_reg(core_if, adpctl.d32);
	dwc_otg_disable_global_interrupts(core_if);

	adpctl.b.adpres = 0;
	adpctl.b.adpen = 1;
	adpctl.b.enasns = 1;
	dwc_otg_adp_write_reg(core_if, adpctl.d32);

	dwc_otg_adp_sense_timer_start(core_if);

	return 0;
}

/**
 * Stops the ADP Probing
 *
 * @param core_if the pointer to core_if strucure.
 */
uint32_t dwc_otg_adp_probe_stop(dwc_otg_core_if_t * core_if)
{

	adpctl_data_t adpctl;
	DWC_DEBUGPL(DBG_ANY, "Stop ADP probe\n");
	core_if->adp.probe_enabled = 0;
	//core_if->adp.probe_counter = 0;
	adpctl.d32 = dwc_otg_adp_read_reg(core_if);

	adpctl.b.adpen = 0;
	adpctl.b.adp_prb_int = 1;
	adpctl.b.adp_tmout_int = 1;
	adpctl.b.adp_sns_int = 1;
	dwc_otg_adp_write_reg(core_if, adpctl.d32);

	return 0;
}

/**
 * Stops the ADP Sensing
 *
 * @param core_if the pointer to core_if strucure.
 */
uint32_t dwc_otg_adp_sense_stop(dwc_otg_core_if_t * core_if)
{
	adpctl_data_t adpctl;

	core_if->adp.sense_enabled = 0;

	adpctl.d32 = dwc_otg_adp_read_reg_filter(core_if);
	adpctl.b.enasns = 0;
	adpctl.b.adp_sns_int = 1;
	dwc_otg_adp_write_reg(core_if, adpctl.d32);

	return 0;
}

/**
 * Called to turn on the VBUS after initial ADP probe in host mode.
 * If port power was already enabled in cil_hcd_start function then
 * only schedule a timer.
 *
 * @param core_if the pointer to core_if structure.
 */
void dwc_otg_adp_turnon_vbus(dwc_otg_core_if_t * core_if)
{
	hprt0_data_t hprt0 = {.d32 = 0 };
	hprt0.d32 = dwc_otg_read_hprt0(core_if);
	DWC_PRINTF("Turn on VBUS for 1.1s, port power is %d\n", hprt0.b.prtpwr);

	if (hprt0.b.prtpwr == 0) {
		hprt0.b.prtpwr = 1;
		//DWC_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
	}

	dwc_otg_adp_vbuson_timer_start(core_if);
}

/**
 * Called right after driver is loaded
 * to perform initial actions for ADP
 *
 * @param core_if the pointer to core_if structure.
 * @param is_host - flag for current mode of operation either from GINTSTS or GPWRDN
 */
void dwc_otg_adp_start(dwc_otg_core_if_t * core_if, uint8_t is_host)
{
	gpwrdn_data_t gpwrdn;

	DWC_DEBUGPL(DBG_ANY, "ADP Initial Start\n");
	core_if->adp.adp_started = 1;

	DWC_WRITE_REG32(&core_if->core_global_regs->gintsts, 0xFFFFFFFF);
	dwc_otg_disable_global_interrupts(core_if);
	if (is_host) {
		DWC_PRINTF("HOST MODE\n");
		//core_if->op_state = A_HOST; - vahrama, modified checking in hcd_start()
		/* Enable Power Down Logic Interrupt*/
		gpwrdn.d32 = 0;
		gpwrdn.b.pmuintsel = 1;
		gpwrdn.b.pmuactv = 1;
		DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, 0, gpwrdn.d32);
		/* Initialize first ADP probe to obtain Ramp Time value */
		core_if->adp.initial_probe = 1;
		dwc_otg_adp_probe_start(core_if);
	} else {
		gotgctl_data_t gotgctl;
		gotgctl.d32 = DWC_READ_REG32(&core_if->core_global_regs->gotgctl);
		DWC_DEBUGPL(DBG_ANY, "DEVICE MODE\n");
		//dwc_otg_core_init(core_if);
		if (gotgctl.b.bsesvld == 0) {
			/* Enable Power Down Logic Interrupt*/
			gpwrdn.d32 = 0;
			DWC_DEBUGPL(DBG_ANY, "VBUS is not valid - start ADP probe\n");
			gpwrdn.b.pmuintsel = 1;
			gpwrdn.b.pmuactv = 1;
			DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, 0, gpwrdn.d32);
			/* Do not need to return to inital probe if we are coming back to
			 * the device mode after HNP */
			if (core_if->op_state != B_HOST)
				core_if->adp.initial_probe = 1;
			dwc_otg_adp_probe_start(core_if);
		} else {
			DWC_PRINTF("VBUS is valid - initialize core as a Device\n");
			core_if->op_state = B_PERIPHERAL;
			//dwc_otg_core_init(core_if);
			dwc_otg_enable_global_interrupts(core_if);
			cil_pcd_start(core_if);
			dwc_otg_dump_global_registers(core_if);
			dwc_otg_dump_dev_registers(core_if);
		}
	}
}

void dwc_otg_adp_init(dwc_otg_core_if_t * core_if)
{
	core_if->adp.adp_started = 0;
	core_if->adp.initial_probe = 0;
	core_if->adp.probe_timer_values[0] = -1;
	core_if->adp.probe_timer_values[1] = -1;
	core_if->adp.probe_enabled = 0;
	core_if->adp.sense_enabled = 0;
	core_if->adp.sense_timer_started = 0;
	core_if->adp.vbuson_timer_started = 0;
	core_if->adp.probe_counter = 0;
	core_if->adp.gpwrdn = 0;
	core_if->adp.attached = DWC_OTG_ADP_UNKOWN;
}

void dwc_otg_adp_remove(dwc_otg_core_if_t * core_if)
{
	gpwrdn_data_t gpwrdn = { .d32 = 0 };
	gpwrdn.b.pmuintsel = 1;
	gpwrdn.b.pmuactv = 1;
	DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, gpwrdn.d32, 0);

	if (core_if->adp.probe_enabled)
		dwc_otg_adp_probe_stop(core_if);
	if (core_if->adp.sense_enabled)
		dwc_otg_adp_sense_stop(core_if);
	if (core_if->adp.sense_timer_started)
		DWC_TIMER_CANCEL(core_if->adp.sense_timer);
	if (core_if->adp.vbuson_timer_started)
		DWC_TIMER_CANCEL(core_if->adp.vbuson_timer);
	DWC_TIMER_FREE(core_if->adp.sense_timer);
	DWC_TIMER_FREE(core_if->adp.vbuson_timer);
}

/////////////////////////////////////////////////////////////////////
////////////// ADP Interrupt Handlers ///////////////////////////////
/////////////////////////////////////////////////////////////////////
/**
 * This function sets Ramp Timer values
 */
static uint32_t set_timer_value(dwc_otg_core_if_t * core_if, uint32_t val)
{
	if (core_if->adp.probe_timer_values[0] == -1) {
		core_if->adp.probe_timer_values[0] = val;
		core_if->adp.probe_timer_values[1] = -1;
		return 1;
	} else {
		core_if->adp.probe_timer_values[1] =
		    core_if->adp.probe_timer_values[0];
		core_if->adp.probe_timer_values[0] = val;
		return 0;
	}
}

/**
 * This function compares Ramp Timer values
 */
static uint32_t compare_timer_values(dwc_otg_core_if_t * core_if)
{
	uint32_t diff;
	uint32_t thres;
	gpwrdn_data_t gpwrdn;

	/* RTIM difference thresold differs for host and device modes */
	gpwrdn.d32 = DWC_READ_REG32(&core_if->core_global_regs->gpwrdn);
	if (!gpwrdn.b.idsts)
		thres = HOST_RTIM_THRESHOLD;
	else
		thres = DEVICE_RTIM_THRESHOLD;

	DWC_DEBUGPL(DBG_ANY, "timer value 0 %d timer value 1 %d\n",
		core_if->adp.probe_timer_values[0], core_if->adp.probe_timer_values[1]);
	if (core_if->adp.probe_timer_values[0] >= core_if->adp.probe_timer_values[1])
		diff = core_if->adp.probe_timer_values[0] - core_if->adp.probe_timer_values[1];
	else
		diff = core_if->adp.probe_timer_values[1] - core_if->adp.probe_timer_values[0];
	if (diff < thres)
		return 0;
	else
		return 1;
}

/**
 * This function handles ADP Probe Interrupts
 */
static int32_t dwc_otg_adp_handle_prb_intr(dwc_otg_core_if_t * core_if,
						 uint32_t val)
{
	adpctl_data_t adpctl = {.d32 = 0 };
	gpwrdn_data_t gpwrdn, temp;
	adpctl.d32 = val;

	temp.d32 = DWC_READ_REG32(&core_if->core_global_regs->gpwrdn);

	core_if->adp.gpwrdn = DWC_READ_REG32(&core_if->core_global_regs->gpwrdn);
	if (adpctl.b.rtim == 0 /*&& !temp.b.idsts*/){
		DWC_PRINTF("RTIM value is 0\n");
		goto exit;
	}
	core_if->adp.probe_counter++;

	if (set_timer_value(core_if, adpctl.b.rtim) &&
	    core_if->adp.initial_probe) {
		core_if->adp.initial_probe = 0;
		dwc_otg_adp_probe_stop(core_if);
		gpwrdn.d32 = 0;
		gpwrdn.b.pmuactv = 1;
		gpwrdn.b.pmuintsel = 1;
		DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, gpwrdn.d32, 0);
		DWC_WRITE_REG32(&core_if->core_global_regs->gintsts, 0xFFFFFFFF);

		/* check which value is for device mode and which for Host mode */
		if (!temp.b.idsts) {	/* considered host mode value is 0 */
			/* Choose right op_state depending on previous one */
			if (core_if->op_state == B_PERIPHERAL)
				core_if->op_state = B_HOST;
			else
				core_if->op_state = A_HOST;
			dwc_otg_enable_global_interrupts(core_if);
			/*
			 * Turn on VBUS after initial ADP probe.
			 */
			DWC_SPINUNLOCK(core_if->lock);
			cil_hcd_start(core_if);
			dwc_otg_adp_turnon_vbus(core_if);
			DWC_SPINLOCK(core_if->lock);
		} else {
			/*
			 * Initiate SRP after initial ADP probe.
			 */
			dwc_otg_enable_global_interrupts(core_if);
			dwc_otg_initiate_srp(core_if);
		}
	} else if (core_if->adp.probe_counter > 2){
		gpwrdn.d32 = DWC_READ_REG32(&core_if->core_global_regs->gpwrdn);
		if (compare_timer_values(core_if)) {
			DWC_PRINTF("Difference in timer values !!! \n");
//          core_if->adp.attached = DWC_OTG_ADP_ATTACHED;
			dwc_otg_adp_probe_stop(core_if);

			/* Power on the core */
			if (core_if->power_down == 2) {
				gpwrdn.b.pwrdnswtch = 1;
				DWC_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, 0, gpwrdn.d32);
			}

			/* check which value is for device mode and which for Host mode */
			if (!temp.b.idsts) {	/* considered host mode value is 0 */
				/* Disable Interrupt from Power Down Logic */
				gpwrdn.d32 = 0;
				gpwrdn.b.pmuintsel = 1;
				gpwrdn.b.pmuactv = 1;
				DWC_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, gpwrdn.d32, 0);

				/*
				 * Initialize the Core for Host mode.
				 * Choose right op_state depending on previous one
				 */
				if (core_if->op_state == B_PERIPHERAL)
					core_if->op_state = B_HOST;
				else
					core_if->op_state = A_HOST;

				dwc_otg_core_init(core_if);
				dwc_otg_enable_global_interrupts(core_if);
				cil_hcd_start(core_if);
				dwc_otg_adp_turnon_vbus(core_if);
			} else {
				gotgctl_data_t gotgctl;
				/* Mask SRP detected interrupt from Power Down Logic */
				gpwrdn.d32 = 0;
				gpwrdn.b.srp_det_msk = 1;
				DWC_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, gpwrdn.d32, 0);

				/* Disable Power Down Logic */
				gpwrdn.d32 = 0;
				gpwrdn.b.pmuintsel = 1;
				gpwrdn.b.pmuactv = 1;
				DWC_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, gpwrdn.d32, 0);

				/*
				 * Initialize the Core for Device mode.
				 */
				core_if->op_state = B_PERIPHERAL;
				//dwc_otg_core_init(core_if);
				cil_pcd_start(core_if);
				dwc_otg_enable_global_interrupts(core_if);

				gotgctl.d32 = DWC_READ_REG32(&core_if->core_global_regs->gotgctl);
				if (!gotgctl.b.bsesvld)
					dwc_otg_initiate_srp(core_if);
			}
		}
		if (core_if->power_down == 2) {
			if (gpwrdn.b.bsessvld) {
				/* Mask SRP detected interrupt from Power Down Logic */
				gpwrdn.d32 = 0;
				gpwrdn.b.srp_det_msk = 1;
				DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, gpwrdn.d32, 0);

				/* Disable Power Down Logic */
				gpwrdn.d32 = 0;
				gpwrdn.b.pmuactv = 1;
				DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, gpwrdn.d32, 0);

				/*
				 * Initialize the Core for Device mode.
				 */
				core_if->op_state = B_PERIPHERAL;
				dwc_otg_core_init(core_if);
				dwc_otg_enable_global_interrupts(core_if);
				cil_pcd_start(core_if);
			}
		}
	}
exit:
	/* Clear interrupt */
	adpctl.d32 = dwc_otg_adp_read_reg(core_if);
	adpctl.b.adp_prb_int = 1;
	dwc_otg_adp_write_reg(core_if, adpctl.d32);

	return 0;
}

/**
 * This function hadles ADP Sense Interrupt
 */
static int32_t dwc_otg_adp_handle_sns_intr(dwc_otg_core_if_t * core_if)
{
	adpctl_data_t adpctl;
	/* Stop ADP Sense timer */
	DWC_TIMER_CANCEL(core_if->adp.sense_timer);

	/* Restart ADP Sense timer */
	dwc_otg_adp_sense_timer_start(core_if);

	/* Clear interrupt */
	adpctl.d32 = dwc_otg_adp_read_reg(core_if);
	adpctl.b.adp_sns_int = 1;
	dwc_otg_adp_write_reg(core_if, adpctl.d32);

	return 0;
}

/**
 * This function handles ADP Probe Interrupts
 */
static int32_t dwc_otg_adp_handle_prb_tmout_intr(dwc_otg_core_if_t * core_if,
						 uint32_t val)
{
	adpctl_data_t adpctl = {.d32 = 0 };
	adpctl.d32 = val;
	set_timer_value(core_if, adpctl.b.rtim);

	/* Clear interrupt */
	adpctl.d32 = dwc_otg_adp_read_reg(core_if);
	adpctl.b.adp_tmout_int = 1;
	dwc_otg_adp_write_reg(core_if, adpctl.d32);

	return 0;
}

/**
 * ADP Interrupt handler.
 *
 */
int32_t dwc_otg_adp_handle_intr(dwc_otg_core_if_t * core_if)
{
	int retval = 0;
	adpctl_data_t adpctl = {.d32 = 0};

	adpctl.d32 = dwc_otg_adp_read_reg(core_if);
	DWC_DEBUGPL(DBG_ANY, "ADPCTL = %08x RAMP TIME = %d\n", adpctl.d32, adpctl.b.rtim);

	if (adpctl.b.adp_sns_int & adpctl.b.adp_sns_int_msk) {
		DWC_DEBUGPL(DBG_ANY, "ADP Sense interrupt\n");
		retval |= dwc_otg_adp_handle_sns_intr(core_if);
	}
	if (adpctl.b.adp_tmout_int & adpctl.b.adp_tmout_int_msk) {
		DWC_DEBUGPL(DBG_ANY, "ADP timeout interrupt\n");
		retval |= dwc_otg_adp_handle_prb_tmout_intr(core_if, adpctl.d32);
	}
	if (adpctl.b.adp_prb_int & adpctl.b.adp_prb_int_msk) {
		DWC_DEBUGPL(DBG_ANY, "ADP Probe interrupt\n");
		adpctl.b.adp_prb_int = 1;
		retval |= dwc_otg_adp_handle_prb_intr(core_if, adpctl.d32);
	}

//	dwc_otg_adp_modify_reg(core_if, adpctl.d32, 0);
	//dwc_otg_adp_write_reg(core_if, adpctl.d32);
	DWC_DEBUGPL(DBG_ANY, "RETURN FROM ADP ISR\n");

	return retval;
}

/**
 *
 * @param core_if Programming view of DWC_otg controller.
 */
int32_t dwc_otg_adp_handle_srp_intr(dwc_otg_core_if_t * core_if)
{

#ifndef DWC_HOST_ONLY
	hprt0_data_t hprt0;
	gpwrdn_data_t gpwrdn;
	DWC_DEBUGPL(DBG_ANY, "++ Power Down Logic Session Request Interrupt++\n");

	gpwrdn.d32 = DWC_READ_REG32(&core_if->core_global_regs->gpwrdn);
	/* check which value is for device mode and which for Host mode */
	if (!gpwrdn.b.idsts) {	/* considered host mode value is 0 */
		DWC_PRINTF("SRP: Host mode\n");

		if (core_if->adp_enable) {
			dwc_otg_adp_probe_stop(core_if);

			/* Power on the core */
			if (core_if->power_down == 2) {
				gpwrdn.b.pwrdnswtch = 1;
				DWC_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, 0, gpwrdn.d32);
			}

			core_if->op_state = A_HOST;
			dwc_otg_core_init(core_if);
			dwc_otg_enable_global_interrupts(core_if);
			cil_hcd_start(core_if);
		}

		/* Turn on the port power bit. */
		hprt0.d32 = dwc_otg_read_hprt0(core_if);
		hprt0.b.prtpwr = 1;
		DWC_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);

		/* Start the Connection timer. So a message can be displayed
		 * if connect does not occur within 10 seconds. */
		cil_hcd_session_start(core_if);
	} else {
		DWC_DEBUGPL(DBG_PCD, "SRP: Device mode %s\n", __FUNCTION__);
		if (core_if->adp_enable) {
			dwc_otg_adp_probe_stop(core_if);

			/* Power on the core */
			if (core_if->power_down == 2) {
				gpwrdn.b.pwrdnswtch = 1;
				DWC_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, 0, gpwrdn.d32);
			}

			gpwrdn.d32 = 0;
			gpwrdn.b.pmuactv = 0;
			DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, 0,
					 gpwrdn.d32);

			core_if->op_state = B_PERIPHERAL;
			dwc_otg_core_init(core_if);
			dwc_otg_enable_global_interrupts(core_if);
			cil_pcd_start(core_if);
		}
	}
#endif
	return 1;
}