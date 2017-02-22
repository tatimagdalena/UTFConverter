/* Fernando Homem da Costa 1211971 33WB */
/* Tatiana de Oliveira Magdalena 1321440 33WB */

#include "utfconv.h"
#include <stdio.h>
#include <stdlib.h>



/******************************************************/
/****** Conversão de UTF-8 para UTF-32(conv8_32) ******/
/******************************************************/

/*
Funcao: Escreve em 32
		Recebe um inteiro e o escreve em um arquivo UTF-32 na ordenacao correta.
Parametros:
		int caractere: inteiro de 4 bytes que sera escrito;
		char ordem: 'L' se for escrito em ordem little-endian ou 'B' se for em ordem big-endian;
		FILE *arq_saida: arquivo onde o inteiro sera escrito.
Retorna:
		0 se a escrita foi efetuada corretamente;
		-1 caso ocorra algum erro.
*/
int escreve32(int caractere, char ordem, FILE *arq_saida)
{
	int i;
	unsigned char temp;
	
	if(ordem == 'B')
	{
		for(i = 3; i >= 0; i--)
		{
			temp = (unsigned char) ((caractere >> (8*i)) & 0xFF);
			if(fwrite(&temp,sizeof(unsigned char),1,arq_saida) != 1)
			{
				fprintf(stderr, "Erro de escrita.\n");
				return -1;
			}
		}
	}
	else if(ordem == 'L')
	{
		for(i = 0; i < 4; i++)
		{
			temp = (unsigned char) ((caractere >> (8*i)) & 0xFF);
			if(fwrite(&temp,sizeof(unsigned char),1,arq_saida) != 1)
			{
				fprintf(stderr, "Erro de escrita.\n");
				return -1;
			}
		}
	}
	else
	{
		fprintf(stderr, "Ordem nao reconhecida.\n");
		return -1;
	}
	return 0;
}

/*
Funcao: Teste de Byte de Continuacao
		Testa se o byte recebido obedece a codificacao de byte de continuacao, ou seja, eh da forma 10xx.xxxx
Parametro:
		char byte: byte que sera analisado.
Retorna:
		0 caso o byte esteja com a codificacao correta;
		-1 caso contrario.
*/
int testeByteCont(char byte)
{
	if( (byte & 0xC0) != 0x80 ) /* (10xx.xxxx & 1100.0000) != 1000.0000 */
	{
		return -1;
	}
	return 0;
}


/*
Funcao: Converte UTF-8 em UTF-32
		Converte um arquivo UTF-8 em outro UTF-32, obedecendo a ordenacao recebida.
Parametros:
		FILE *arq_entrada: arquivo em UTF-8;
		FILE *arq_saida: arquivo em UTF-32 na ordenacao indicada;
		char ordem: 'L' se for escrito em ordem little-endian ou 'B' se for em ordem big-endian;
Retorna:
		0 caso o arquivo seja gravado corretamente;
		-1 caso contrario.
*/
int conv8_32(FILE* arq_entrada, FILE* arq_saida, char ordem)
{
	unsigned int BOM = 0x0000FEFF;
	unsigned char byte[4]; /* Vetor em que cada posicao eh o i-esimo byte lido do caractere */
	unsigned char mask[4]; /* Vetor em que cada posicao eh uma mascara verificar se o caractere se utiliza de (pos+1) bytes */

	unsigned int caractere = 0; /* Caractere que sera escrito no arquivo de saida */
	unsigned int aux = 0; /* Inteiro auxiliar para escrita */
	int i = -1; /* Contador de bytes lidos */
	int res = 0; /* Variavel para testar os erros */
	
/* Mascara para saber quantos bytes sao utilizados no caractere, observando o byte mais significativo */
	mask[0] = 0x80; /* (1000.0000 & 0xxx.xxxx) == 0 */
	mask[1] = 0xE0; /* (1110.0000 & 110x.xxxx) == 0xC0 */
	mask[2] = 0xF0; /* (1111.0000 & 1110.xxxx) == 0xE0 */
	mask[3] = 0xF8; /* (1111.1000 & 1111.0xxx) == 0xF0 */


/* Imprime caractere BOM no inicio do arquivo de saida */
	res = escreve32(BOM,ordem,arq_saida);
	if(res == -1)
	{
		return res;
	}

/* Le os caracteres contidos no arquivo, enquanto eles existirem (leitura byte a byte)*/	
	while(fread(byte,sizeof(unsigned char),1,arq_entrada) == 1)
	{
		i++;
		
		/* Caso que usa apenas um byte */
		if((byte[0] & mask[0]) == 0x00)
		{

			/* Monta o inteiro que sera escrito */
			caractere = (int) byte[0];

			res = escreve32(caractere,ordem,arq_saida);
			if(res == -1)
			{
				return res;
			}
		}

		/* Caso que usa dois bytes, precisa ler mais um */
		else if((byte[0] & mask[1]) == 0xC0) 
		{
			if(fread(byte+1,sizeof(unsigned char),1,arq_entrada) != 1)
			{
				/* Se nao conseguiu ler, e ainda nao atingiu o final do arquivo, sinal de que foi erro de leitura */
				if(feof(arq_entrada) == 0)
				{
					fprintf(stderr,"Erro de leitura.\n");
					return -1;
				}
				/* Se nao conseguiu ler por ja estar no final do arquivo, sinal de que foi erro de codificacao */
				else
				{
					fprintf(stderr,"Erro de codificacao no %d-esimo byte.\n",i);
					return -1;
				}
			}
			
			/*Testes de codificacao dos bytes intermediarios, um a um para poder saber seu indice caso seja detectado erro */
			i++;
			res = testeByteCont(byte[1]);
			if(res == -1)
			{
				fprintf(stderr,"Erro de codificacao no %d-esimo byte.\n",i);
				return res;
			}

			/* Desliga bytes de marcacao */
			byte[0] &= 0x1F; /* 110x.xxxx -> 000x.xxxx */
			byte[1] &= 0x3F; /* 10yy.yyyy -> 00yy.yyyy */

			/* Monta o inteiro que sera escrito */
			caractere = (int) byte[0];  /* Copia o byte1 para um inteiro vazio:             0000.0000 0000.0000 0000.0000 000x.xxxx */
			caractere <<= 6;            /* Coloca o byte1 em seu lugar:                     0000.0000 0000.0000 0000.0xxx xx00.0000 */
			caractere |= (int) byte[1]; /* Passa o byte menos significativo para o inteiro: 0000.0000 0000.0000 0000.0xxx xxyy.yyyy */
			
			res = escreve32(caractere,ordem,arq_saida);
			if(res == -1)
			{
				return res;
			}
		}
		
		/* Caso que usa tres bytes, precisa ler mais dois */
		else if((byte[0] & mask[2]) == 0xE0) 
		{
			if(fread(byte+1,sizeof(unsigned char),2,arq_entrada) != 2)
			{
				/* Se nao conseguiu ler, e ainda nao atingiu o final do arquivo, sinal de que foi erro de leitura */
				if(feof(arq_entrada) == 0)
				{
					fprintf(stderr,"Erro de leitura.\n");
					return -1;
				}
				/* Se nao conseguiu ler por ja estar no final do arquivo, sinal de que foi erro de codificacao */
				else
				{
					fprintf(stderr,"Erro de codificacao no %d-esimo byte.\n",i);
					return -1;
				}
			}
			
			/*Testes de codificacao dos bytes intermediarios, um a um para poder saber seu indice caso seja detectado erro */
			i++;
			res = testeByteCont(byte[1]);
			if(res == -1)
			{
				fprintf(stderr,"Erro de codificacao no %d-esimo byte\n",i);
				return res;
			}
			i++;
			res = testeByteCont(byte[2]);
			if(res == -1)
			{
				fprintf(stderr,"Erro de codificacao no %d-esimo byte\n",i);
				return res;
			}
			
			/* Desliga bytes de marcacao */
			byte[0] &= 0x0F; /* 1110.xxxx -> 0000.xxxx */
			byte[1] &= 0x3F; /* 10yy.yyyy -> 00yy.yyyy */
			byte[2] &= 0x3F; /* 10zz.zzzz -> 00zz.zzzz */
			
			/* Monta o inteiro que sera escrito */
			caractere = (int) byte[0]; /* Copia o byte1 para um inteiro vazio:                      0000.0000 0000.0000 0000.0000 0000.xxxx */
			caractere <<= 12;   /* Coloca o byte1 em seu lugar:                                     0000.0000 0000.0000 xxxx.0000 0000.0000 */
			aux = (int) byte[1] << 6; /* Passa o byte2 para um inteiro para coloca-lo em seu lugar: 0000.0000 0000.0000 0000.yyyy yy00.0000 */
			caractere |= aux; /* Passa o byte2, ja em seu lugar, para o inteiro com o byte1:        0000.0000 0000.0000 xxxx.yyyy yy00.0000 */
			caractere |= (int) byte[2]; /* Passa o byte menos significativo para o inteiro:         0000.0000 0000.0000 xxxx.yyyy yyzz.zzzz */
			
			res = escreve32(caractere,ordem,arq_saida);
			if(res == -1)
			{
				return res;
			}
		}
		
		/* Caso que usa quatro bytes, precisa ler mais tres */
		else if((byte[0] & mask[3]) == 0xF0) 
		{
			if(fread(byte+1,sizeof(unsigned char),3,arq_entrada) != 3)
			{
				/* Se nao conseguiu ler, e ainda nao atingiu o final do arquivo, sinal de que foi erro de leitura */
				if(feof(arq_entrada) == 0)
				{
					fprintf(stderr,"Erro de leitura.\n");
					return -1;
				}
				/* Se nao conseguiu ler por ja estar no final do arquivo, sinal de que foi erro de codificacao */
				else
				{
					fprintf(stderr,"Erro de codificacao no %d-esimo byte.\n",i);
					return -1;
				}
			}

			/*Testes de codificacao dos bytes intermediarios, um a um para poder saber seu indice caso seja detectado erro */
			i++;
			res = testeByteCont(byte[1]);
			if(res == -1)
			{
				fprintf(stderr,"Erro de codificacao no %d-esimo byte\n",i);
				return res;
			}
			i++;
			res = testeByteCont(byte[2]);
			if(res == -1)
			{
				fprintf(stderr,"Erro de codificacao no %d-esimo byte\n",i);
				return res;
			}
			i++;
			res = testeByteCont(byte[3]);
			if(res == -1)
			{
				fprintf(stderr,"Erro de codificacao no %d-esimo byte\n",i);
				return res;
			}

			/* Desliga bytes de marcacao */
			byte[0] &= 0x07; /* 1111.0xxx -> 0000.0xxx */
			byte[1] &= 0x3F; /* 10yy.yyyy -> 00yy.yyyy */
			byte[2] &= 0x3F; /* 10zz.zzzz -> 00zz.zzzz */
			byte[3] &= 0x3F; /* 10rr.rrrr -> 00rr.rrrr */
			
			/* Monta o inteiro que sera escrito */
			caractere = (int) byte[0]; /* Copia o byte1 para um inteiro vazio:                       0000.0000 0000.0000 0000.0000 0000.0xxx */
			caractere <<= 18;          /* Coloca o byte1 em seu lugar:                               0000.0000 000x.xx00 0000.0000 0000.0000 */
			aux = (int) byte[1] << 12; /* Passa o byte2 para um inteiro para coloca-lo em seu lugar: 0000.0000 0000.00yy yyyy.0000 0000.0000 */
			caractere |= aux;         /* Passa o byte2, ja em seu lugar, para o inteiro com o byte1: 0000.0000 000x.xxyy yyyy.0000 0000.0000 */
			aux = (int) byte[2] << 6;  /* Passa o byte3 para um inteiro para coloca-lo em seu lugar: 0000.0000 0000.0000 0000.zzzz zz00.0000 */
			aux |= (int) byte[3];   /* Passa o byte menos significativo para o inteiro auxiliar:     0000.0000 0000.0000 0000.zzzz zzrr.rrrr */
			caractere |= aux;        /* Junta os bytes dos dois inteiros utilizados:                 0000.0000 000x.xxyy yyyy.zzzz zzrr.rrrr */
						
			res = escreve32(caractere,ordem,arq_saida);
			if(res == -1)
			{
				return res;
			}
		}

		/* Caso em que o primeiro byte do caractere nao corresponde a nenhuma codificacao possivel */
		else
		{
			fprintf(stderr,"Erro de codificacao no %d-esimo byte\n",i);
			return -1;
		}
	}

	/* Acabou de ler, testa se o final do arquivo foi realmente atingindo */
	if(feof(arq_entrada) == 0) /* Caso em que nao foi atingido o final do arquivo */
	{
		fprintf(stderr,"Erro de leitura.\n");
		return -1;
	}

	return 0;
}





/******************************************************/
/****** Conversão de UTF-32 para UTF-8(conv32_8) ******/
/******************************************************/


/*
Funcao: Cria Inteiro
		Recebe um vetor de bytes e cria um inteiro de acordo com a ordem estabelecida.
Parametros:
		unsigned char *byte: vetor de 4 posicoes, em que cada posicao contem um byte lido, na ordem em que foi lido.
		char ordem: 'L' se for para ser escrito em ordem little-endian ou 'B' se for em ordem big-endian;
Retorna:
		Um unsigned int contendo o inteiro gerado.
*/
unsigned int criaInt(unsigned char *byte, char ordem)
{
	int i, j;
	/* b0: xxxx.xxxx b1: yyyy.yyyy b2: wwww.wwww b3:zzzz.zzzz */
	unsigned int caractere = 0, aux = 0;

	if(ordem == 'L')
	{
		caractere = (unsigned int) byte[0]; 
		for(i = 1; i < 4; i++)
		{
			aux = ((unsigned int) byte[i]) << 8*i;
			caractere |= aux;
		} /* zzzz.zzzz wwww.wwww yyyy.yyyy xxxx.xxxx */
	}
	else
	{
		caractere = (unsigned int) byte[3];

		for(i = 1, j = 2; ((i < 4) && (j >= 0)) ; i++, j--)
		{
			aux = ((unsigned int) byte[j]) << 8 * i;
			caractere |= aux;
		} /* xxxx.xxxx yyyy.yyyy wwww.wwww zzzz.zzzz */
	}

	return caractere;
}


/*
Funcao: Converte UTF-32 em UTF-8
		Converte um arquivo UTF-32 em outro UTF-8.
Parametros:
		FILE *arq_entrada: arquivo em UTF-32;
		FILE *arq_saida: arquivo em UTF-8;
Retorna:
		0 caso o arquivo seja gravado corretamente;
		-1 caso contrario.
*/
int conv32_8(FILE *arq_entrada, FILE *arq_saida)
{
	int i = 0; /* Contador de bytes lidos */
	char ordem;
	unsigned int BOM;
	unsigned char byte[4] = {0,0,0,0};
	unsigned char mask2, mask3, mask4; /* Maskb liga os bits de marcacao do byte inicial da representacao que utiliza b bytes */
	unsigned char maskCont; /* Mascara que liga o bit inicial de marcacao de um byte de continuacao */
	unsigned int caractere;
	unsigned char saida[4]; /* saida[pos] é o (pos+1)-esimo byte a ser escrito, contando os 4 bytes lidos */

	mask2 = 0xC0;
	mask3 = 0xE0;
	mask4 = 0xF0;
	maskCont = 0x80;

	/* Lendo o BOM */
	if(fread(byte,sizeof(unsigned char),4,arq_entrada) != 4)
	{
		/* Se nao conseguiu ler, e ainda nao atingiu o final do arquivo, sinal de que foi erro de leitura */
		if(feof(arq_entrada) == 0)
		{
			fprintf(stderr,"Erro de leitura.\n");
			return -1;
		}
		/* Se nao conseguiu ler por ja estar no final do arquivo, sinal de que foi erro de codificacao */
		else
		{
			fprintf(stderr,"Erro de codificacao no 1o byte.\n");
			return -1;
		}
	}

	if((byte[0] == 0x00) && (byte[1] == 0x00) && (byte[2] == 0xFE) && (byte[3] == 0xFF))
		ordem = 'B';
	else if((byte[0] == 0xFF) && (byte[1] == 0xFE) && (byte[2] == 0x00) && (byte[3] == 0x00))
		ordem = 'L';
	else
	{
		fprintf(stderr, "Erro de codificacao no 1o byte.\n");
		return -1;
	}
	
	i += 4;

	while(fread(byte,sizeof(unsigned char),1,arq_entrada) == 1)
	{
		i++;

		if(fread(byte+1,sizeof(unsigned char),3,arq_entrada) != 3)
		{
			/* Se nao conseguiu ler, e ainda nao atingiu o final do arquivo, sinal de que foi erro de leitura */
			if(feof(arq_entrada) == 0)
			{
				fprintf(stderr,"Erro de leitura.\n");
				return -1;
			}
			/* Se nao conseguiu ler por ja estar no final do arquivo, sinal de que foi erro de codificacao */
			else
			{
				fprintf(stderr,"Erro de codificacao no %d-esimo byte.\n",i);
				return -1;
			}
		}
		i += 3;

		caractere = criaInt(byte,ordem);

		/* Caso em que ocupa apenas um byte: 0xxx.xxxx */
		if( (0x0000 < caractere) && (caractere < 0x007F) )
		{
			saida[0] = (unsigned char) caractere;

			if(fwrite(&saida[0],sizeof(unsigned char),1,arq_saida) != 1)
			{
				fprintf(stderr,"Erro de gravacao.\n");
				return -1;
			}
		}

		/* Caso em que ocupa 2 bytes: 110x.xxxx xxxx.xxxx */
		else if( (0x0080 < caractere) && (caractere < 0x07FF) ) /* 1000 0000 a 0111 1111 1111 */
		{
			saida[0] = ((unsigned char) (caractere >> 6)) | mask2;
			saida[1] = ((unsigned char) (caractere & 0x3F)) | maskCont;
			
			if(fwrite(saida,sizeof(unsigned char),2,arq_saida) != 2)
			{
				fprintf(stderr,"Erro de gravacao.\n");
				return -1;
			}
		}

		/* Caso em que ocupa 3 bytes: 1110.xxxx 10xx.xxxx 10xx.xxxx */
		else if( (0x0800 < caractere) && (caractere < 0xFFFF) ) /* 0000.1000 0000.0000 a 1111.1111 1111.1111 */
		{
			saida[0] = ((unsigned char) (caractere >> 12)) | mask3;
			saida[1] = ((unsigned char) ((caractere >> 6) & 0x3F)) | maskCont;
			saida[2] = ((unsigned char) (caractere & 0x3F)) | maskCont;

			if(fwrite(saida,sizeof(unsigned char),3,arq_saida) != 3)
			{
				fprintf(stderr,"Erro de gravacao.\n");
				return -1;
			}
		}

		/* Caso em que ocupa 4 bytes: 1111.0xxx 10xx.xxxx 10xx.xxxx 10xx.xxxx */
		else if( (0x10000 < caractere) && (caractere < 0x10FFFF) ) /* 0000.0001 0000.0000 0000.0000 a 0001.0000 1111.1111 1111.1111 */
		{
			saida[0] = ((unsigned char) (caractere >> 18)) | mask4;
			saida[1] = ((unsigned char) ((caractere >> 12) & 0x3F)) | maskCont;
			saida[2] = ((unsigned char) ((caractere >> 6) & 0x3F)) | maskCont;
			saida[3] = ((unsigned char) (caractere & 0x3F)) | maskCont;

			if(fwrite(saida,sizeof(unsigned char),4,arq_saida) != 4)
			{
				fprintf(stderr,"Erro de gravacao.\n");
				return -1;
			}
		}

		/* Caso em que o valor esta fora do que eh possivel ser armazenado em UTF-8 */
		else
		{
			fprintf(stderr,"Erro de codificacao no %d-esimo byte.\n",i);
			return -1;
		}
	}

	/* Acabou de ler, testa se o final do arquivo foi realmente atingindo */
	if(feof(arq_entrada) == 0) /* Caso em que nao foi atingido o final do arquivo */
	{
		fprintf(stderr,"Erro de leitura.\n");
		return -1;
	}

	return 0;
}