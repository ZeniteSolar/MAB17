#include "can_app.h"

/**
 * @brief Prints a can message via usart
 */
inline void can_app_print_msg(can_t *msg)
{
    usart_send_string("ID: ");
    usart_send_uint16(msg->id);
    usart_send_string(". D: ");

    for(uint8_t i = 0; i < msg->length; i++){
      usart_send_uint16(msg->data[i]);
      usart_send_char(' ');
    }

    usart_send_string(". ERR: ");
    can_error_register_t err = can_read_error_register();
    usart_send_uint16(err.rx);
    usart_send_char(' ');
    usart_send_uint16(err.tx);
    usart_send_char('\n');
}

/**
* @brief Manages the canbus application protocol
*/
inline void can_app_task(void)
{
    check_can();

    if(can_app_send_state_clk_div++ >= CAN_APP_SEND_STATE_CLK_DIV){
        VERBOSE_MSG_CAN_APP(usart_send_string("state msg was sent.\n"));
        can_app_send_state();
        can_app_send_state_clk_div = 0;
    }

    if(can_app_send_pumps_clk_div++ >= CAN_APP_SEND_PUMPS_CLK_DIV){
        VERBOSE_MSG_CAN_APP(usart_send_string("pumps msg was sent.\n"));
        can_app_send_pumps();
        can_app_send_pumps_clk_div = 0;
    }

}

inline void can_app_send_state(void)
{
    can_t msg;
    msg.id                                  = CAN_FILTER_MSG_MAM17_STATE;
    msg.length                              = CAN_LENGTH_MSG_STATE;

    msg.data[CAN_SIGNATURE_BYTE]            = CAN_SIGNATURE_SELF;
    msg.data[CAN_STATE_MSG_STATE_BYTE]      = (uint8_t) state_machine;
    msg.data[CAN_STATE_MSG_ERROR_BYTE]      = error_flags.all;

    can_send_message(&msg);
}

inline void can_app_send_pumps(void)
{
    can_t msg;

    msg.id                                  = CAN_FILTER_MSG_MAB17_PUMPS;
    msg.length                              = CAN_LENGTH_MSG_MAB17_PUMPS;

    for(uint8_t i = msg.length; i; i--)     msg.data[i-1] = 0;

    msg.data[CAN_SIGNATURE_BYTE]            = CAN_SIGNATURE_SELF;
    msg.data[CAN_MSG_MAB17_PUMPS_BYTE]      |= 
        ((system_flags.pump1_on) << CAN_MSG_MAB17_PUMP1_BIT);

    can_app_print_msg(&msg);
    can_send_message(&msg); 
}

/**
 * @brief extracts the specific MIC17 STATE message
 * @param *msg pointer to the message to be extracted
 */
inline void can_app_extractor_mic17_state(can_t *msg)
{
    // TODO:
    //  - se tiver em erro, desligar acionamento
    if(msg->data[CAN_SIGNATURE_BYTE] == CAN_SIGNATURE_MIC17){
        // zerar contador
        if(msg->data[CAN_STATE_MSG_ERROR_BYTE]){
            //ERROR!!!
        }
        /*if(contador == maximo)*/{
            //ERROR!!!
        }

         
    }
}
 
/**
 * @brief extracts the specific MIC17 PUMPS message
 *
 * @param *msg pointer to the message to be extracted
*/ 
inline void can_app_extractor_mic17_pumps(can_t *msg)
{
    if(msg->data[CAN_SIGNATURE_BYTE] == CAN_SIGNATURE_MIC17){

        can_app_print_msg(msg);
        
        can_app_checks_without_mic17_msg = 0;

        system_flags.pump1_on       = bit_is_set(msg->data[
            CAN_MSG_MIC17_PUMPS_PUMPS_BYTE], 
            CAN_MSG_MIC17_PUMPS_PUMP1_BIT);

        system_flags.pump2_on       = bit_is_set(msg->data[
            CAN_MSG_MIC17_PUMPS_PUMPS_BYTE], 
            CAN_MSG_MIC17_PUMPS_PUMP2_BIT);

        system_flags.pump3_on       = bit_is_set(msg->data[
            CAN_MSG_MIC17_PUMPS_PUMPS_BYTE], 
            CAN_MSG_MIC17_PUMPS_PUMP3_BIT);

    }
}

/**
 * @brief redirects a specific message extractor to a given message
 * @param *msg pointer to the message to be extracted
 */
inline void can_app_msg_extractors_switch(can_t *msg)
{
    if(msg->data[CAN_SIGNATURE_BYTE] == CAN_SIGNATURE_MIC17){
        switch(msg->id){
            case CAN_FILTER_MSG_MIC17_PUMPS:
                VERBOSE_MSG_CAN_APP(usart_send_string("got a pumps msg: "));
                VERBOSE_MSG_CAN_APP(can_app_print_msg(msg));
                can_app_extractor_mic17_pumps(msg);
                break;
            case CAN_FILTER_MSG_MIC17_STATE:
                VERBOSE_MSG_CAN_APP(usart_send_string("got a state msg: "));
                VERBOSE_MSG_CAN_APP(can_app_print_msg(msg));
                can_app_extractor_mic17_state(msg);
                break;
            default:
                VERBOSE_MSG_CAN_APP(usart_send_string("got a unknown msg: "));
                VERBOSE_MSG_CAN_APP(can_app_print_msg(msg));
                break;
        }    
    }
}

/**
 * @brief Manages to receive and extract specific messages from canbus
 */
inline void check_can(void)
{
    // If no messages is received from mic17 for
    // CAN_APP_CHECKS_WITHOUT_MIC17_MSG cycles, than it go to a specific error state. 
    //VERBOSE_MSG_CAN_APP(usart_send_string("checks: "));
    //VERBOSE_MSG_CAN_APP(usart_send_uint16(can_app_checks_without_mic17_msg));
    if(can_app_checks_without_mic17_msg++ >= CAN_APP_CHECKS_WITHOUT_MIC17_MSG){
        VERBOSE_MSG_CAN_APP(usart_send_string("Error: too many cycles withtou message.\n"));
        can_app_checks_without_mic17_msg = 0;
        error_flags.no_canbus = 1;
        //set_state_error();
    }
    
    if(can_check_message()){
        can_t msg;
        if(can_get_message(&msg)){
            can_app_msg_extractors_switch(&msg);
        }
    }
}

