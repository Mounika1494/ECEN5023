/*
 * i2c_TSL2561.h
 *
 *  Created on: Sep 29, 2016
 *      Author: Mounika Reddy
 */

#ifndef I2C_TSL2561_H_
#define I2C_TSL2561_H_
#endif /* I2C_TSL2561_H_ */
#define I2C_CTRL 0x0
#define I2C_THL0 0x02
#define I2C_THL1 0x03
#define I2C_THH0 0x04
#define I2C_THH1 0x05
#define I2C_INT  0x06
#define ADC0_LOW 0x0C
#define ADC0_HIGH 0x0D
#define I2C_TIMING 0x01
/**values of register**/
#define THL0 0x0F
#define THL1 0x00
#define THH0 0x00
#define THH1 0x08
#define PERSIST 0x4
#define INT_Reg 0x1
#define Power_up 0x03
#define TIMING_Value 0x01
uint8_t I2C_Write(uint8_t value);
uint8_t I2C_Read();
void i2c_Reg(uint8_t reg_value,uint8_t addr_reg);








