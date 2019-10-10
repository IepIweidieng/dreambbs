/*-------------------------------------------------------*/
/* rfc2047.c            ( NTHU CS MapleBBS Ver 3.10 )    */
/*-------------------------------------------------------*/
/* target : RFC 2047 QP/base64 encode                    */
/* create : 03/04/11                                     */
/* update : 03/05/19                                     */
/* author : PaulLiu.bbs@bbs.cis.nctu.edu.tw              */
/*-------------------------------------------------------*/

#include <stdio.h>

/*-------------------------------------------------------*/
/* RFC2047 QP encode                                     */
/*-------------------------------------------------------*/

void
output_rfc2047_qp(FILE * fp,
                  const char *prefix, const char *str, const char *charset, const char *suffix)
{
    int i, ch;
    int blank = 1;                /* 1:���Ѫťղզ� */
    static char tbl[16] =
        { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',
'E', 'F' };

    fputs(prefix, fp);

    /* �p�G�r��}�Y�� US_ASCII printable characters�A�i�����X�A�o�ˤ���n�ݡA�]����ۮe */
    for (i = 0; (ch = str[i]); i++)
    {
        if (ch != '=' && ch != '?' && ch != '_' && ch > '\x1f' && ch < '\x7f')
        {
            if (blank)
            {
                if (ch != ' ')
                    blank = 0;
                else if (str[i + 1] == '\0')    /* �Y���O�ťաA�̫�@�ӭn��X */
                    break;
            }
            fprintf(fp, "%c", ch);
        }
        else
            break;
    }

    if (ch != '\0')                /* �p�G���S���S��r���N���� */
    {
        /* �}�l encode */
        fprintf(fp, "=?%s?Q?", charset);    /* ���w�r�� */
        for (; (ch = str[i]); i++)
        {
            /* �p�G�O non-printable �r���N�n��X */
            /* �d��: '\x20' ~ '\x7e' �� printable, �䤤 =, ?, _, �ť�, ���S��Ÿ��]�n��X */

            if (ch == '=' || ch == '?' || ch == '_' || ch <= '\x1f'
                || ch >= '\x7f')
                fprintf(fp, "=%c%c", tbl[(ch >> 4) & '\x0f'],
                        tbl[ch & '\x0f']);
            else if (ch == ' ')    /* �ťդ���S��, �ন '_' �� "=20" */
                fprintf(fp, "=20");
            else
                fprintf(fp, "%c", ch);
        }
        fputs("?=", fp);
    }

    fputs(suffix, fp);
}
