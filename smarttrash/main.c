#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <errno.h>

#define TRIG_GPIO "28"  // P9_12 -> GPIO 28
#define ECHO_GPIO "16"  // P9_15 -> GPIO 16

#define I2C_BUS "/dev/i2c-2"
#define LCD_ADDR 0x27

#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE 0x04
#define LCD_RS 0x01

#define PULSE_DELAY 500
#define CMD_DELAY 1000
#define INIT_DELAY 4500
#define TEXT_DELAY 1000000

#define PWM_PERIOD "20000000" // 20ms para servo

void lcd_pulse(int fd, uint8_t data) {
    uint8_t buf[1] = {data};
    write(fd, buf, 1);
    usleep(PULSE_DELAY);
    buf[0] = data | LCD_ENABLE;
    write(fd, buf, 1);
    usleep(PULSE_DELAY);
    buf[0] = data;
    write(fd, buf, 1);
    usleep(PULSE_DELAY);
}

int lcd_send(int fd, uint8_t data, uint8_t mode) {
    uint8_t high_nibble = (data & 0xF0) | LCD_BACKLIGHT | mode;
    uint8_t low_nibble = ((data << 4) & 0xF0) | LCD_BACKLIGHT | mode;
    lcd_pulse(fd, high_nibble);
    lcd_pulse(fd, low_nibble);
    usleep(CMD_DELAY);
    return 0;
}

int lcd_init(int fd) {
    usleep(INIT_DELAY);
    uint8_t init_sequence[] = {0x03, 0x03, 0x03, 0x02};
    for (int i = 0; i < 4; i++) {
        lcd_send(fd, init_sequence[i], 0);
        usleep(INIT_DELAY);
    }
    lcd_send(fd, 0x28, 0);
    lcd_send(fd, 0x08, 0);
    lcd_send(fd, 0x01, 0);
    lcd_send(fd, 0x06, 0);
    lcd_send(fd, 0x0C, 0);
    return 0;
}

int lcd_print(int fd, const char *str) {
    while (*str) {
        lcd_send(fd, (uint8_t)(*str), LCD_RS);
        str++;
    }
    return 0;
}

void lcd_set_cursor(int fd, int row, int col) {
    int row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    lcd_send(fd, 0x80 | (row_offsets[row] + col), 0);
}

void lcd_clear_line(int fd, int row) {
    lcd_set_cursor(fd, row, 0);
    for (int i = 0; i < 16; i++) {
        lcd_print(fd, " ");
    }
    lcd_set_cursor(fd, row, 0);
}

int configurar_pwm(const char *caminho, const char *valor) {
    FILE *fp = fopen(caminho, "w");
    if (fp == NULL) {
        perror("Erro ao acessar o caminho");
        return 1;
    }
    fprintf(fp, "%s", valor);
    fclose(fp);
    return 0;
}

void export_gpio(const char *gpio) {
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd == -1) {
        perror("Erro ao exportar GPIO");
        exit(1);
    }
    write(fd, gpio, strlen(gpio));
    close(fd);
}

void set_gpio_direction(const char *gpio, const char *direction) {
    char path[50];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%s/direction", gpio);
    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror("Erro ao configurar direção do GPIO");
        exit(1);
    }
    write(fd, direction, strlen(direction));
    close(fd);
}

void write_gpio(const char *gpio, int value) {
    char path[50];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%s/value", gpio);
    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror("Erro ao escrever no GPIO");
        exit(1);
    }
    dprintf(fd, "%d", value);
    close(fd);
}

int read_gpio(const char *gpio) {
    char path[50], value;
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%s/value", gpio);
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("Erro ao ler GPIO");
        exit(1);
    }
    read(fd, &value, 1);
    close(fd);
    return (value == '1') ? 1 : 0;
}

float medir_distancia() {
    // Envia pulso para o TRIG
    write_gpio(TRIG_GPIO, 1);
    usleep(10);
    write_gpio(TRIG_GPIO, 0);

    // Espera o ECHO ir para HIGH
    while (read_gpio(ECHO_GPIO) == 0);

    // Mede o tempo que o ECHO fica em HIGH
    struct timeval start, end;
    gettimeofday(&start, NULL);
    while (read_gpio(ECHO_GPIO) == 1);
    gettimeofday(&end, NULL);

    // Calcula a distância em cm
    long elapsed_time = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    float distancia = (elapsed_time * 0.0343) / 2;

    return distancia;
}

void mover_servo(int angulo) {
    char caminho[50];
    snprintf(caminho, sizeof(caminho), "/sys/class/pwm/pwmchip0/pwm0/duty_cycle");
    int duty_cycle = (angulo * 1000000 / 180) + 500000; // Converte ângulo para duty cycle
    char valor[20];
    snprintf(valor, sizeof(valor), "%d", duty_cycle);
    configurar_pwm(caminho, valor);
}

int main() {
    // Configura GPIOs
    export_gpio(TRIG_GPIO);
    export_gpio(ECHO_GPIO);
    set_gpio_direction(TRIG_GPIO, "out");
    set_gpio_direction(ECHO_GPIO, "in");

    // Configura PWM para o servo
    configurar_pwm("/sys/class/pwm/pwmchip0/export", "0");
    configurar_pwm("/sys/class/pwm/pwmchip0/pwm0/period", PWM_PERIOD);
    configurar_pwm("/sys/class/pwm/pwmchip0/pwm0/enable", "1");

    // Inicializa LCD
    int lcd_fd = open(I2C_BUS, O_RDWR);
    if (lcd_fd < 0) {
        perror("Erro ao abrir o barramento I2C");
        exit(1);
    }
    if (ioctl(lcd_fd, I2C_SLAVE, LCD_ADDR) < 0) {
        perror("Erro ao configurar endereço do LCD");
        exit(1);
    }
    lcd_init(lcd_fd);

    // Inicia com a tampa fechada
    mover_servo(0);
    lcd_print(lcd_fd, "  HI I'M WALLE");
    lcd_clear_line(lcd_fd, 1);
    lcd_set_cursor(lcd_fd, 1, 1);
    lcd_print(lcd_fd, "Tampa Fechada");

    while (1) {
        float distancia = medir_distancia();
        if (distancia < 10.0) { // Se objeto estiver a menos de 10 cm
            mover_servo(180); // Abre a tampa
            lcd_clear_line(lcd_fd, 1);
            lcd_set_cursor(lcd_fd, 1, 1);
            lcd_print(lcd_fd, "Jogue seu lixo");
            usleep(TEXT_DELAY);
        } else {
            mover_servo(0); // Fecha a tampa
        }
        usleep(500000); // Espera 300ms antes de medir novamente
        lcd_clear_line(lcd_fd, 1);
        lcd_set_cursor(lcd_fd, 1, 1);
        lcd_print(lcd_fd, "Tampa Fechada");
    }
    close(lcd_fd);
    return 0;
}