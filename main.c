#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

void put_char_bits_int_file(char n, int fd)
{
	/*
		int(нужен численый тип для побитовых операций, char не подходит) - 4 байта,
		мне нужен оттуда только последний. Готовлю последний байт вот так - 0x10000000
	*/
	const int unit = 1 << 8; // Операция сдвига еденицы на 8 разрядов влево

	/*
		Поразрядно иду по параметру n и узнаю, стоит ли 1 в каждом разряде или нет
	*/
	for (int i = 0; i < 8; i++)
	{
		if (n & unit)
			write(fd, "1", 1);
		else
			write(fd, "0", 1);
		n = n << 1; // Первый разряд прочитан, перехожу к следующему
	}
	write(fd, "\n", 1);
}

void put_str_bits_in_file(char *str, int fd, int str_len)
{
	for (int i = 0; i < str_len; i++)
		put_char_bits_int_file(str[i], fd);
}

/*
	Функция проверяет, будет ли переполнение char при сложении параметров
*/
int handle(unsigned char a, unsigned char b)
{
  	if (a > 255 - b)
    	return 1;
	return 0;
}

/*
	Считываю инфу из под флагов. Флаги не проверяются на повтор
*/
void read_params(int *capacity, char **file_name, char **argv)
{
	for (int i = 1; ; i++)
	{
		if (argv[i] && argv[i][0] == '-' && argv[i][1] == 'f' && argv[i][2] == '\0')
			*file_name = argv[++i];
		else if (argv[i] && argv[i][0] == '-' && argv[i][1] == 'n' && argv[i][2] == '\0')
			*capacity = atoi(argv[++i]);
		else
		{
			if (*capacity == 512 && *file_name == NULL)
				printf("Input error. Default settings will be used\n");
			return ;
		}
	}
}

int tmp_to_sourse(char *file_name)
{
    int fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);   // Открываем фалы по новой,
    int tmp_fd = open("tmp", O_RDONLY, 0644);    // только меняем их местами на запись и чтение

    if (tmp_fd == -1)
        return (-1);
	if (fd == -1)
		fd = 1;
    char c;
    int retval;
    while(1) // Переписываем из временного файла в исходный результат
    {
        retval = read(tmp_fd, &c, 1);
        if (retval == -1)
            return (-1);
        if (retval == 0)
            break;
        write(fd, &c, 1);
    }
    close(fd);
    close(tmp_fd);
    return (0);
}

/*
	Вынес из main кусок кода, открываю необходимые файлы и выделяю память
*/
int resource_preparation(char **str, char *file_name, int *fd, int *tmp_fd, int capasity)
{
	*fd = open(file_name, O_RDONLY, 0644);
	if (*fd == -1)
		*fd = 0;

	*tmp_fd = open("tmp", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (*tmp_fd == -1)
	{
		printf("Open file error\n");
		return -1;
	}

	*str = (char *)malloc(sizeof(char) * capasity);
	if (!(*str))
	{
		printf("Memory allocation error\n");
		return -1;
	}
	return (0);
}

int main(int argc, char **argv)
{
	argc = 0; //Заглушаю warning на неиспользуемую переменную

	int capacity = 512;
	char *file_name = NULL;
	int fd;
	int tmp_fd;
	char *str;

	read_params(&capacity, &file_name, argv);

	if (resource_preparation(&str, file_name, &fd, &tmp_fd, capacity) == -1)
		return -1;

	// Где расставил такие надписи - доп код для проверки. Его можно закоментить или удалить,
	// если появится какой-то другой способ сделать это

	/* For check. Start */
	int chk_fd = open("check", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (chk_fd == -1)
	{
		printf("Open file error\n");
		return -1;
	}/* For check. End */

	int retval;
	char c;
	char rank;
	while((retval = read(fd, str, capacity)) > 0)
	{
		/* For check. Start */
		write(chk_fd, "BASE\n", 6);
		put_str_bits_in_file(str, chk_fd, retval);
		/* For check. End */

		// По алгоритму "дополнения" необходимо ивертировать все биты числа и прибавить к 
		// результату 1. Количество разрядов может быть очень большим, поэтому считываю данные
		// из файла по одному байту, инвертирую его, добавляю 1 и сохраняю.
		// Но может возникнуть проблемма, когда входящий байт будет вот таким - 0х11111111,
		// тогда после прибавления единицы(что требует алгоритм дополнения), один бит потеряется
		// из-за переполнения. Поэтому его нужно перенести на следующий байт, за это отвечает
		// переменная rank.
		rank = 1;
		for (int i = retval - 1; i >= 0; i--)
		{
			c = ~str[i] + rank;
			if (handle(~str[i], rank))
				rank = 2;
			else
				rank = 1;
			str[i] = c;
		}

		/* For check. Start */
		write(chk_fd, "ADDITION\n", 10);
		put_str_bits_in_file(str, chk_fd, retval);
		/* For check. Start */

		write(tmp_fd, str, retval);
	}
	if (fd != 0)
		close(fd);
	close(tmp_fd);
	/* For check. Start */
	close(chk_fd);
	/* For check. Start */

	if (tmp_to_sourse(file_name) == -1)
	{
		printf("Reopen file error\n");
		return -1;
	}

	return 0;
}
