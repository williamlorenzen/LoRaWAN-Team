/***************************************************************************//**
* \file I2C_I2C_INT.c
* \version 4.0
*
* \brief
*  This file provides the source code to the Interrupt Service Routine for
*  the SCB Component in I2C mode.
*
* Note:
*
********************************************************************************
* \copyright
* Copyright 2013-2017, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "I2C_PVT.h"
#include "I2C_I2C_PVT.h"
#include "cyapicallbacks.h"


/*******************************************************************************
* Function Name: I2C_I2C_ISR
****************************************************************************//**
*
*  Handles the Interrupt Service Routine for the SCB I2C mode.
*
*******************************************************************************/
CY_ISR(I2C_I2C_ISR)
{
    uint32 diffCount;
    uint32 endTransfer;

#ifdef I2C_I2C_ISR_ENTRY_CALLBACK
    I2C_I2C_ISR_EntryCallback();
#endif /* I2C_I2C_ISR_ENTRY_CALLBACK */

#if (I2C_I2C_CUSTOM_ADDRESS_HANDLER_CONST)
    uint32 response;

    response = I2C_I2C_ACK_ADDR;
#endif /* (I2C_I2C_CUSTOM_ADDRESS_HANDLER_CONST) */

    endTransfer = 0u; /* Continue active transfer */

    /* Calls customer routine if registered */
    if(NULL != I2C_customIntrHandler)
    {
        I2C_customIntrHandler();
    }

    if(I2C_CHECK_INTR_I2C_EC_MASKED(I2C_INTR_I2C_EC_WAKE_UP))
    {
        /* Mask-off after wakeup */
        I2C_SetI2CExtClkInterruptMode(I2C_NO_INTR_SOURCES);
    }

    /* Master and Slave error tracking:
    * Add the master state check to track only the master errors when the master is active or
    * track slave errors when the slave is active or idle.
    * A special MMS case: in the address phase with misplaced Start: the master sets the LOST_ARB and
    * slave BUS_ERR. The valid event is LOST_ARB comes from the master.
    */

    #if(I2C_I2C_MASTER)
    {
        /* INTR_MASTER_I2C_BUS_ERROR:
        * A misplaced Start or Stop condition occurred on the bus: complete the transaction.
        * The interrupt is cleared in I2C_FSM_EXIT_IDLE.
        */
        if(I2C_CHECK_INTR_MASTER_MASKED(I2C_INTR_MASTER_I2C_BUS_ERROR))
        {
            I2C_mstrStatus |= (uint16) (I2C_I2C_MSTAT_ERR_XFER |
                                                     I2C_I2C_MSTAT_ERR_BUS_ERROR);

            endTransfer = I2C_I2C_CMPLT_ANY_TRANSFER;
        }

        /* INTR_MASTER_I2C_ARB_LOST:
        * The MultiMaster lost arbitrage during transaction.
        * A Misplaced Start or Stop condition is treated as lost arbitration when the master drives the SDA.
        * The interrupt source is cleared in I2C_FSM_EXIT_IDLE.
        */
        if(I2C_CHECK_INTR_MASTER_MASKED(I2C_INTR_MASTER_I2C_ARB_LOST))
        {
            I2C_mstrStatus |= (uint16) (I2C_I2C_MSTAT_ERR_XFER |
                                                     I2C_I2C_MSTAT_ERR_ARB_LOST);

            endTransfer = I2C_I2C_CMPLT_ANY_TRANSFER;
        }


        /* The error handling common part:
        * Sets a completion flag of the master transaction and passes control to:
        *  - I2C_FSM_EXIT_IDLE - to complete transaction in case of: ARB_LOST or BUS_ERR.
        *  - I2C_FSM_IDLE      - to take chance for the slave to process incoming transaction.
        */
        if(0u != endTransfer)
        {
            /* Set completion flags for master */
            I2C_mstrStatus |= (uint16) I2C_GET_I2C_MSTAT_CMPLT;

            /* In case of LOST*/
                I2C_state = I2C_I2C_FSM_EXIT_IDLE;
        }
    }
    #endif


    /* States description:
    * Any Master operation starts from: the ADDR_RD/WR state as the master generates traffic on the bus.
    * Any Slave operation starts from: the IDLE state as the slave always waits for actions from the master.
    */

    /* FSM Master */
    if(I2C_CHECK_I2C_FSM_MASTER)
    {
        #if(I2C_I2C_MASTER)
        {
            /* INTR_MASTER_I2C_STOP:
            * A Stop condition was generated by the master: the end of the transaction.
            * Set completion flags to notify the API.
            */
            if(I2C_CHECK_INTR_MASTER_MASKED(I2C_INTR_MASTER_I2C_STOP))
            {
                I2C_ClearMasterInterruptSource(I2C_INTR_MASTER_I2C_STOP);

                I2C_mstrStatus |= (uint16) I2C_GET_I2C_MSTAT_CMPLT;
                I2C_state       = I2C_I2C_FSM_IDLE;
            }
            else
            {
                if(I2C_CHECK_I2C_FSM_ADDR) /* Address stage */
                {
                    /* INTR_MASTER_I2C_NACK:
                    * The master sent an address but it was NACKed by the slave. Complete transaction.
                    */
                    if(I2C_CHECK_INTR_MASTER_MASKED(I2C_INTR_MASTER_I2C_NACK))
                    {
                        I2C_ClearMasterInterruptSource(I2C_INTR_MASTER_I2C_NACK);

                        I2C_mstrStatus |= (uint16) (I2C_I2C_MSTAT_ERR_XFER |
                                                                 I2C_I2C_MSTAT_ERR_ADDR_NAK);

                        endTransfer = I2C_I2C_CMPLT_ANY_TRANSFER;
                    }
                    /* INTR_TX_UNDERFLOW. The master sent an address:
                    *  - TX direction: the clock is stretched after the ACK phase, because the TX FIFO is
                    *    EMPTY. The TX EMPTY cleans all the TX interrupt sources.
                    *  - RX direction: the 1st byte is received, but there is no ACK permission,
                    *    the clock is stretched after 1 byte is received.
                    */
                    else
                    {
                        if(I2C_CHECK_I2C_FSM_RD) /* Reading */
                        {
                            I2C_state = I2C_I2C_FSM_MSTR_RD_DATA;
                        }
                        else /* Writing */
                        {
                            I2C_state = I2C_I2C_FSM_MSTR_WR_DATA;
                            if(0u != I2C_mstrWrBufSize)
                            {
                                /* Enable INTR.TX_EMPTY if there is data to transmit */
                                I2C_SetTxInterruptMode(I2C_INTR_TX_EMPTY);
                            }
                        }
                    }
                }

                if(I2C_CHECK_I2C_FSM_DATA) /* Data phase */
                {
                    if(I2C_CHECK_I2C_FSM_RD) /* Reading */
                    {
                        /* INTR_RX_FULL:
                        * RX direction: the master received 8 bytes.
                        * Get data from RX FIFO and decide whether to ACK or  NACK the following bytes.
                        */
                        if(I2C_CHECK_INTR_RX_MASKED(I2C_INTR_RX_FULL))
                        {
                            /* Calculate difference */
                            diffCount =  I2C_mstrRdBufSize -
                                        (I2C_mstrRdBufIndex + I2C_GET_RX_FIFO_ENTRIES);

                            /* Proceed transaction or end it when RX FIFO becomes FULL again */
                            if(diffCount > I2C_I2C_FIFO_SIZE)
                            {
                                diffCount = I2C_I2C_FIFO_SIZE;
                            }
                            else
                            {
                                if(0u == diffCount)
                                {
                                    I2C_DISABLE_MASTER_AUTO_DATA_ACK;

                                    diffCount   = I2C_I2C_FIFO_SIZE;
                                    endTransfer = I2C_I2C_CMPLT_ANY_TRANSFER;
                                }
                            }

                            for(; (0u != diffCount); diffCount--)
                            {
                                I2C_mstrRdBufPtr[I2C_mstrRdBufIndex] = (uint8)
                                                                                        I2C_RX_FIFO_RD_REG;
                                I2C_mstrRdBufIndex++;
                            }
                        }
                        /* INTR_RX_NOT_EMPTY:
                        * RX direction: the master received one data byte, ACK or NACK it.
                        * The last byte is stored and NACKed by the master. The NACK and Stop is
                        * generated by one command generate Stop.
                        */
                        else if(I2C_CHECK_INTR_RX_MASKED(I2C_INTR_RX_NOT_EMPTY))
                        {
                            /* Put data in component buffer */
                            I2C_mstrRdBufPtr[I2C_mstrRdBufIndex] = (uint8) I2C_RX_FIFO_RD_REG;
                            I2C_mstrRdBufIndex++;

                            if(I2C_mstrRdBufIndex < I2C_mstrRdBufSize)
                            {
                                I2C_I2C_MASTER_GENERATE_ACK;
                            }
                            else
                            {
                               endTransfer = I2C_I2C_CMPLT_ANY_TRANSFER;
                            }
                        }
                        else
                        {
                            /* Do nothing */
                        }

                        I2C_ClearRxInterruptSource(I2C_INTR_RX_ALL);
                    }
                    else /* Writing */
                    {
                        /* INTR_MASTER_I2C_NACK :
                        * The master writes data to the slave and NACK was received: not all the bytes were
                        * written to the slave from the TX FIFO. Revert the index if there is data in
                        * the TX FIFO and pass control to a complete transfer.
                        */
                        if(I2C_CHECK_INTR_MASTER_MASKED(I2C_INTR_MASTER_I2C_NACK))
                        {
                            I2C_ClearMasterInterruptSource(I2C_INTR_MASTER_I2C_NACK);

                            /* Rollback write buffer index: NACKed byte remains in shifter */
                            I2C_mstrWrBufIndexTmp -= (I2C_GET_TX_FIFO_ENTRIES +
                                                                   I2C_GET_TX_FIFO_SR_VALID);

                            /* Update number of transferred bytes */
                            I2C_mstrWrBufIndex = I2C_mstrWrBufIndexTmp;

                            I2C_mstrStatus |= (uint16) (I2C_I2C_MSTAT_ERR_XFER |
                                                                     I2C_I2C_MSTAT_ERR_SHORT_XFER);

                            I2C_CLEAR_TX_FIFO;

                            endTransfer = I2C_I2C_CMPLT_ANY_TRANSFER;
                        }
                        /* INTR_TX_EMPTY :
                        * TX direction: the TX FIFO is EMPTY, the data from the buffer needs to be put there.
                        * When there is no data in the component buffer, the underflow interrupt is
                        * enabled to catch when all the data has been transferred.
                        */
                        else if(I2C_CHECK_INTR_TX_MASKED(I2C_INTR_TX_EMPTY))
                        {
                            while(I2C_I2C_FIFO_SIZE != I2C_GET_TX_FIFO_ENTRIES)
                            {
                                /* The temporary mstrWrBufIndexTmp is used because slave could NACK the byte and index
                                * roll-back required in this case. The mstrWrBufIndex is updated at the end of transfer.
                                */
                                if(I2C_mstrWrBufIndexTmp < I2C_mstrWrBufSize)
                                {
                                #if(!I2C_CY_SCBIP_V0)
                                   /* Clear INTR_TX.UNDERFLOW before putting the last byte into TX FIFO. This ensures
                                    * a proper trigger at the end of transaction when INTR_TX.UNDERFLOW single trigger
                                    * event. Ticket ID# 156735.
                                    */
                                    if(I2C_mstrWrBufIndexTmp == (I2C_mstrWrBufSize - 1u))
                                    {
                                        I2C_ClearTxInterruptSource(I2C_INTR_TX_UNDERFLOW);
                                        I2C_SetTxInterruptMode(I2C_INTR_TX_UNDERFLOW);
                                    }
                                 #endif /* (!I2C_CY_SCBIP_V0) */

                                    /* Put data into TX FIFO */
                                    I2C_TX_FIFO_WR_REG = (uint32) I2C_mstrWrBufPtr[I2C_mstrWrBufIndexTmp];
                                    I2C_mstrWrBufIndexTmp++;
                                }
                                else
                                {
                                    break; /* No more data to put */
                                }
                            }

                        #if(I2C_CY_SCBIP_V0)
                            if(I2C_mstrWrBufIndexTmp == I2C_mstrWrBufSize)
                            {
                                I2C_SetTxInterruptMode(I2C_INTR_TX_UNDERFLOW);
                            }

                            I2C_ClearTxInterruptSource(I2C_INTR_TX_ALL);
                        #else
                            I2C_ClearTxInterruptSource(I2C_INTR_TX_EMPTY);
                        #endif /* (I2C_CY_SCBIP_V0) */
                        }
                        /* INTR_TX_UNDERFLOW:
                        * TX direction: all data from the TX FIFO was transferred to the slave.
                        * The transaction needs to be completed.
                        */
                        else if(I2C_CHECK_INTR_TX_MASKED(I2C_INTR_TX_UNDERFLOW))
                        {
                            /* Update number of transferred bytes */
                            I2C_mstrWrBufIndex = I2C_mstrWrBufIndexTmp;

                            endTransfer = I2C_I2C_CMPLT_ANY_TRANSFER;
                        }
                        else
                        {
                            /* Do nothing */
                        }
                    }
                }

                if(0u != endTransfer) /* Complete transfer */
                {
                    /* Clean-up master after reading: only in case of NACK */
                    I2C_DISABLE_MASTER_AUTO_DATA_ACK;

                    /* Disable data processing interrupts: they have to be cleared before */
                    I2C_SetRxInterruptMode(I2C_NO_INTR_SOURCES);
                    I2C_SetTxInterruptMode(I2C_NO_INTR_SOURCES);

                    if(I2C_CHECK_I2C_MODE_NO_STOP(I2C_mstrControl))
                    {
                        /* On-going transaction is suspended: the ReStart is generated by the API request */
                        I2C_mstrStatus |= (uint16) (I2C_I2C_MSTAT_XFER_HALT |
                                                                 I2C_GET_I2C_MSTAT_CMPLT);

                        I2C_state = I2C_I2C_FSM_MSTR_HALT;
                    }
                    else
                    {
                        /* Complete transaction: exclude the data processing state and generate Stop.
                        * The completion status will be set after Stop generation.
                        * A special case is read: because NACK and Stop are generated by the command below.
                        * Lost arbitration can occur during NACK generation when
                        * the other master is still reading from the slave.
                        */
                        I2C_I2C_MASTER_GENERATE_STOP;
                    }
                }
            }

        } /* (I2C_I2C_MASTER) */
        #endif

    } /* (I2C_CHECK_I2C_FSM_MASTER) */

    /* FSM EXIT:
    * Slave:  INTR_SLAVE_I2C_BUS_ERROR, INTR_SLAVE_I2C_ARB_LOST
    * Master: INTR_MASTER_I2C_BUS_ERROR, INTR_MASTER_I2C_ARB_LOST.
    */
    else
    {
        I2C_I2CFwBlockReset();
        
    }

}


/* [] END OF FILE */
