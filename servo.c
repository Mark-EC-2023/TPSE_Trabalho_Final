#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define SLAVE_ADDRESS_LCD 0x27
#define I2C_DEVICE_FILE_PATH "/dev/i2c-2"

short lcd1602_write(uint8_t address, uint8_t *pData, uint8_t len);
short lcd1602_init(void);
void lcd1602_sendCommand(char command);
void lcd1602_sendData(uint8_t data);
void lcd1602_sendString(char *str);
void lcd1602_setCursorPosition(bool row, int column);
void lcd1602_clear(void);

/ Configurações do LCD /
#define LCD_CLEAR_DISPLAY 0x01
#define LCD_ENTRY_MODE_SET 0x04
#define LCD_DISPLAY_CONTROL 0x08
#define LCD_FUNCTION_SET 0x20
#define LCD_RS (1 << 0)
#define LCD_EN (1 << 2)
#define LCD_BACK_LIGHT (1 << 3)

int fd;

short lcd1602_write(uint8_t address, uint8_t *pData, uint8_t len) {
    int ret;
    char buf[len + 1];
    buf[0] = address;
    for (int i = 1; i < (len + 1); i++) {
        buf[i] = *(pData + (i - 1));
    }

    ret = write(fd, buf, (len + 1));
    if (ret <= 0) {
        perror("Erro na escrita no LCD");
        return -1;
    }

    return 0;
}

void lcd1602_sendCommand(char command) {
    uint8_t commandBuf[4];
    commandBuf[0] = (command & 0xF0) | LCD_EN | LCD_BACK_LIGHT;
    commandBuf[1] = (command & 0xF0) | LCD_BACK_LIGHT;
    commandBuf[2] = ((command << 4) & 0xF0) | LCD_EN | LCD_BACK_LIGHT;
    commandBuf[3] = ((command << 4) & 0xF0) | LCD_BACK_LIGHT;

    lcd1602_write(SLAVE_ADDRESS_LCD, commandBuf, 4);
}

void lcd1602_sendData(uint8_t data) {
    uint8_t dataBuf[4];
    dataBuf[0] = (data & 0xF0) | LCD_EN | LCD_BACK_LIGHT | LCD_RS;
    dataBuf[1] = (data & 0xF0) | LCD_BACK_LIGHT | LCD_RS;
    dataBuf[2] = ((data << 4) & 0xF0) | LCD_EN | LCD_BACK_LIGHT | LCD_RS;
    dataBuf[3] = ((data << 4) & 0xF0) | LCD_BACK_LIGHT | LCD_RS;

    lcd1602_write(SLAVE_ADDRESS_LCD, dataBuf, 4);
}

void lcd1602_clear(void) {
    lcd1602_sendCommand(LCD_CLEAR_DISPLAY);
    usleep(2000);
}

void lcd1602_setCursorPosition(bool row, int column) {
    if (row) {
        column |= 0xC0; // Linha 2
    } else {
        column |= 0x80; // Linha 1
    }
    lcd1602_sendCommand(column);
}

short lcd1602_init(void) {
    lcd1602_sendCommand(LCD_FUNCTION_SET | 0x08);
    usleep(1000);
    lcd1602_sendCommand(LCD_DISPLAY_CONTROL);
    usleep(1000);
    lcd1602_sendCommand(LCD_CLEAR_DISPLAY);
    usleep(2000);
    lcd1602_sendCommand(LCD_ENTRY_MODE_SET | 0x02);
    usleep(1000);
    lcd1602_sendCommand(LCD_DISPLAY_CONTROL | 0x04);
    return 0;
}

void lcd1602_sendString(char *str) {
    while (*str) {
        lcd1602_sendData(*str++);
    }
}

int main() {
    // Abrir o dispositivo I2C
    fd = open(I2C_DEVICE_FILE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Erro ao abrir o dispositivo I2C");
        return -1;
    }

    // Configurar o endereço do escravo
    if (ioctl(fd, I2C_SLAVE, SLAVE_ADDRESS_LCD) < 0) {
        perror("Erro ao configurar o endereço do escravo");
        close(fd);
        return -1;
    }

    // Inicializar o LCD
    lcd1602_init();
    lcd1602_clear();

    // Mensagem inicial no LCD
    lcd1602_setCursorPosition(0, 5); // Linha 1, Coluna 5
    lcd1602_sendString("TPSE II"); // Mensagem na linha 1
    lcd1602_setCursorPosition(1, 3); // Linha 2, Coluna 3
    lcd1602_sendString("DIGITE ALGO:");

    // Interação com o usuário
    char input[17]; // Máximo de 16 caracteres por linha
    while (1) {
        printf("\nDigite uma mensagem (máx. 16 caracteres por linha): ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            perror("Erro ao ler entrada");
            break;
        }

        // Remover o caractere de nova linha, se presente
        input[strcspn(input, "\n")] = '\0';

        lcd1602_clear(); // Limpa o LCD antes de exibir a nova mensagem
        lcd1602_setCursorPosition(0, 0);
        lcd1602_sendString(input);

        if (strlen(input) > 16) {
            lcd1602_setCursorPosition(1, 0);
            lcd1602_sendString(&input[16]); // Exibir parte restante na linha 2
        }
    }
close(fd);
    return 0;
}

