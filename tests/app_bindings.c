/* Exercise the REAL application byte ring, mailbox, reply scheduler and UART
 * callbacks. Only HAL/RTOS effects are replaced; no simulated Madgwick path. */
#define FLIGHT_APP_MODE 1
#include "../Core/Src/flight_app.c"
#include <stdlib.h>

UART_HandleTypeDef huart1={USART1,0,0};
TIM_HandleTypeDef htim4={4};
unsigned OSRunning=1;
static uint32_t fake_us;
static uint8_t transmitted[96];
static uint16_t transmitted_size;
unsigned app_rx_starts,app_tx_starts,app_tx_aborts;
uint32_t BSP_TimeUs(void) { return fake_us; }
uint32_t HAL_GetTick(void) { if(mock_busy) fake_us+=1000; return fake_us/1000; }
void Error_Handler(void) { abort(); }
unsigned SEGGER_RTT_WriteString(unsigned ch,const char *s) { (void)ch; (void)s; return 0; }
OS_EVENT *OSSemCreate(unsigned value) { static OS_EVENT event; event.value=value; return &event; }
unsigned OSSemPost(OS_EVENT *s) { (void)s; return 0; }
unsigned OSSemAccept(OS_EVENT *s) { (void)s; return 0; }
void OSSemPend(OS_EVENT *s,unsigned timeout,INT8U *err) { (void)s; (void)timeout; *err=0; }
void OSTimeDly(unsigned ticks) { (void)ticks; }
unsigned OSTaskCreateExt(void (*fn)(void *),void *arg,OS_STK *top,unsigned prio,unsigned id,
                        OS_STK *bottom,unsigned size,void *ext,unsigned opt) {
    (void)fn; (void)arg; (void)top; (void)prio; (void)id; (void)bottom; (void)size; (void)ext; (void)opt; return 0;
}
void OSTaskNameSet(unsigned p,INT8U *name,INT8U *err) { (void)p; (void)name; *err=0; }
unsigned OSTaskStkChk(unsigned p,OS_STK_DATA *s) { (void)p; s->OSFree=500; return 0; }
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *h) { (void)h; return HAL_OK; }
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *h,uint8_t *p,uint16_t n) {
    (void)p; (void)n; h->RxState=HAL_UART_STATE_BUSY; ++app_rx_starts; return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *h,uint8_t *p,uint16_t n) {
    if(h->gState!=HAL_UART_STATE_READY) return HAL_BUSY;
    memcpy(transmitted,p,n); transmitted_size=n; h->gState=HAL_UART_STATE_BUSY; ++app_tx_starts; return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_AbortTransmit(UART_HandleTypeDef *h) {
    ++app_tx_aborts; h->gState=HAL_UART_STATE_READY; return HAL_OK;
}
void app_reset(void) {
    fake_us=0; memset(&filter,0,sizeof(filter)); memset(&published,0,sizeof(published));
    memset(&command_parser,0,sizeof(command_parser));
    command_pending=reply_pending=quiet_slot=tx_busy=attitude_pending=telemetry_pending=0;
    rx_read=rx_write=0; rx_overflow=rx_errors=rx_bytes=release_missed=tx_dropped=0;
    warmup_samples=last_sample_us=initialized=have_time=0;
    app_rx_starts=app_tx_starts=app_tx_aborts=0;
    huart1.gState=huart1.RxState=HAL_UART_STATE_READY;
    transmitted_size=0; Flight_Start(); poll_commands();
}
void app_receive(const uint8_t *bytes,unsigned n) {
    while(n--) { rx_byte=*bytes++; huart1.RxState=HAL_UART_STATE_READY; HAL_UART_RxCpltCallback(&huart1); }
}
void app_cycle(uint32_t now) {
    fake_us=now; handle_cal_command(); process_sample(); publish();
}
void app_telemetry(void) { FlightSnapshot s; telemetry_step(&s); }
void app_tx_complete(void) { huart1.gState=HAL_UART_STATE_READY; HAL_UART_TxCpltCallback(&huart1); }
void app_rx_error(void) { huart1.RxState=HAL_UART_STATE_READY; HAL_UART_ErrorCallback(&huart1); }
unsigned app_frame(uint8_t out[96]) { memcpy(out,transmitted,transmitted_size); return transmitted_size; }
unsigned app_pending(void) { return tx_busy; }
unsigned app_rx_drops(void) { return rx_overflow; }
void app_invalid_config(void) { config_ok=0; }
