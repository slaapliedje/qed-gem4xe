#include "global.h"
#include "set.h"

/* bits[] was the byte-at-a-time bit table setincl/setin and friends used;
 * they index the longs directly now (see the note above setincl), and
 * nothing else wanted it. */

/*****************************************************************************/

short setfree(SET set)
{
	short	i;
	unsigned long help, h;

	h = 0xFFFFFFFFL;
	for (i = 0; i < SETMAX; i += 32)
		if ((help = *set++) != h)
		{
			h = 0x80000000L;
			while (help >= h)
			{
				i++; 
				help <<= 1;
			}
			break;
		}
	return i;
}

/*****************************************************************************/

short setmax(SET set)
{
	short	i;
	unsigned long help;

	for (i = SETSIZE; (--i) >= 0; )
		if ((help = set[i]) != 0L)
		{
			i *= 32;
			for (i+=31; (help&1L)==0; i--,help>>=1) ;
			return (i);
		}
	return -1;
}

/*****************************************************************************/

short setmin(SET set)
{
	short	i;
	unsigned long help, h;

	for (i=0; i<SETMAX; i+=32)
		if ((help=*set++)!=0L)
		{
			h = 0x80000000L;
			while (help<h)
			{
				i++; help<<=1;
			}
			break;
		}
	return i;
}

/*****************************************************************************/

void setcpy (SET set1, SET set2)
{
	memcpy(set1, set2, SETSIZE * (short) sizeof(unsigned long));
}

/*****************************************************************************/
#if (SETSIZE&1)!=0
	So gehts nicht
#endif

void setall (SET set)
{
	short i;

	i = SETSIZE/2;
	while ((--i)>=0)
	{
		*set++ = 0xFFFFFFFFL;
		*set++ = 0xFFFFFFFFL;
	}
}

/*****************************************************************************/

void setclr (SET set)
{
	short i;

	i = SETSIZE/2;
	while ((--i) >= 0)
	{
		*set++ = 0L;
		*set++ = 0L;
	}
}

/*****************************************************************************/

void setnot (SET set)
{
	short i;

	i = SETSIZE/2;
	while ((--i)>=0)
	{
		*set++ ^= 0xFFFFFFFFL;
		*set++ ^= 0xFFFFFFFFL;
	}
}

/*****************************************************************************/

void setand (SET set1, SET set2)
{
	short i;

	i = SETSIZE/2;
	while ((--i)>=0)
	{
		*set1++ &= *set2++;
		*set1++ &= *set2++;
	}
}

/*****************************************************************************/

void setor (SET set1, SET set2)
{
	short i;

	i = SETSIZE/2;
	while ((--i)>=0)
	{
		*set1++ |= *set2++;
		*set1++ |= *set2++;
	}
}

/*****************************************************************************/

void setxor (SET set1, SET set2)
{
	short i;

	i = SETSIZE/2;
	while ((--i)>=0)
	{
		*set1++ ^= *set2++;
		*set1++ ^= *set2++;
	}
}

/*****************************************************************************/

/*
 * ENDIANNESS.  These four reached the set through a char pointer --
 *
 *	*((char *)set + (elt >> 3)) |= bits[elt & 7];
 *
 * -- which puts element 0 in BYTE 0, bit 7.  setfree(), setmin() and
 * setmax() read the same storage as `unsigned long` and take element 0 to
 * be BIT 31 of the first long: they shift left until the value reaches
 * 0x80000000, counting as they go.  The two views agree only where byte 0
 * of a long is its most significant byte -- on a big-endian machine like
 * the 68000.
 *
 * On a little-endian one, which the 65C816 this is ported to is, they do
 * not.  setincl(s,1) set bit 7 of the long; setmin(s) and setmax(s) then
 * both answered 24, and setin(s,24) was false.  restore_edit() walks
 * chg_links in exactly that way -- setmin() to setmax(), testing setin()
 * -- so its loop body never ran once: every change char_insert queued was
 * recorded and none of them was ever drawn.  A typed character went into
 * the document and nothing appeared on the screen.
 *
 * Indexing the longs directly makes the two views agree whatever the byte
 * order, and keeps the numbering the long-based functions already use, so
 * nothing else in this file changes.
 */
#define SET_WORD(elt)	((short)((elt) >> 5))
#define SET_MASK(elt)	(0x80000000UL >> ((elt) & 31))

void setincl (SET set, short elt)
{
	if (elt >= 0 && elt <= SETMAX)
		set[SET_WORD(elt)] |= SET_MASK(elt);
}

/*****************************************************************************/

void setexcl (SET set, short elt)
{
	if (elt>=0 && elt<=SETMAX)
		set[SET_WORD(elt)] &= ~SET_MASK(elt);
}

/*****************************************************************************/

void setchg (SET set, short elt)
{
	if (elt>=0 && elt<=SETMAX)
		set[SET_WORD(elt)] ^= SET_MASK(elt);
}

/*****************************************************************************/

bool setin (SET set, short elt)
{
	if (elt >= 0 && elt <= SETMAX)
		return ((set[SET_WORD(elt)] & SET_MASK(elt)) ? TRUE : FALSE);
	else
		return (FALSE);
}

/*****************************************************************************/

bool setcmp (SET set1, SET set2)
{
	short i;

	if (set2==NULL)
		for (i=0; i<SETSIZE && *set1++==0; i++);
	else
		for (i=0; i<SETSIZE && *set1++==*set2++; i++);
	return (i==SETSIZE);
} 

/*****************************************************************************/

short setcard (SET set)
{
	short i, card, max;

	max = setmax(set);
	for (i=setmin(set), card=0; i<=max; i++)
		if (setin (set, i)) card++;

	return (card);
}

/*****************************************************************************/

void str2set (char *str, SET set)
{
	short	i;

	setclr(set);
	i = 0;
	while(str[i])
	{
		if (str[i]=='-' && i > 0)
		{
			char c;

			i++;
			for (c = str[i - 2]; c < str[i]; c++)
				setincl(set,c);
		}
		else
			setincl(set,str[i++]);
	}
}
