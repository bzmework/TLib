/*
ģ�飺 cstrul.c
���ܣ� ת��unsigned long����ֵΪ�ַ�����
������ val��Ҫת����unsigned long����ֵ��
	  out������ת������ַ�������unsigned long����ֵ�Ļ��棬���С������12*sizeof(tchar)���ֽڡ�
���أ� ָ��out��ָ�롣
˵���� �����ͬ�Ľ���Ӧ����printf/sprintf�����
�汾�� 2019-05-20 denglf ģ�鴴��
*/

#include "../typedef.h"
#include "../typecvt.h"

tchar* cstrul(ulong val, tchar* out)
{
	tchar* left = out;	//���λ��
	tchar* p = out;		//��ǰλ��
	ulong n;			//��ǰ����
	tchar c;			//��ǰ�ַ�

	//������ת�����ַ�
	do
	{
		n = val % 10; //ȡ�ø�λ
		val /= 10; //ȥ����λ
		*p++ = _c_tochr(n);//ת����'0'-'9'֮����ַ�
	} while (val > 0);
	*p-- = NUL;//�������ӽ�����������ָ��

	//��ת'-'��'\0'֮�������Ϊ��ȷ��˳��
	do
	{
		c = *left;
		*left++ = *p;
		*p-- = c;
	} while (left < p);

	//����
	return out;
}