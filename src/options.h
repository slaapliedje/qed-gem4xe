#ifndef _qed_options_h_
#define _qed_options_h_

/* QED_NEAR -- gem4xe port.  A few of these globals are used in a pointer
 * DIFFERENCE (p - klammer_auf in block.c) or otherwise reached in a way
 * that makes Calypsi emit the array's address as a 16-bit immediate.
 * Under --data-model=large the array is in a far bank, so that immediate
 * overflows at link (Calypsi's far arithmetic is 16-bit: it should emit
 * .word0, but range-checks the whole 24-bit address).  Placing just these
 * few small arrays near keeps the arithmetic genuinely 16-bit and the
 * relocation valid.  Guarded, so the m68k build is unchanged. */
#ifndef QED_NEAR
# ifdef __CALYPSI__
#  define QED_NEAR __near
# else
#  define QED_NEAR
# endif
#endif

/* 
 * Autosave 
*/
extern bool	as_text, as_prj;						/* Autosave ? */
extern bool	as_text_ask, as_prj_ask;			/*  nachfragen? */
extern short	as_text_min, as_prj_min;			/*  Minuten */

extern void		set_autosave_options(void);

/* 
 * Globale Optionen 
*/
extern bool	clip_on_disk, wind_cycle, f_to_desk,
				save_opt, save_win, overwrite, blinking_cursor, ctrl_mark_mode,
				olga_autostart, emu_klammer;
extern bool syntax_active;
extern short	transfer_size, bin_line_len;
extern short	fg_color, bg_color;
extern short fg_block_color, bg_block_color;
extern PATH	helpprog;
#define BIN_ANZ	10
extern char	bin_extension[BIN_ANZ][MASK_LEN+1];

extern void		set_global_options	(void);
extern void		set_syntax_options	(void);

/*
 * Klammerpaare
*/
extern QED_NEAR char	klammer_auf[],
				klammer_zu[];

extern void	set_klammer_options(void);


/* 
 * Lokale Optionen 
*/

/* Anzahl der lokalen Optionen */
#define LOCAL_ANZ	20
extern LOCOPT	local_options[LOCAL_ANZ];

extern void		set_local_options(void);


/* 
 * Datei 
*/
extern void 	write_cfg_str		(char *var, char *value);
extern void 	write_cfg_int		(char *var, short value);
extern void 	write_cfg_long		(char *var, long value);
extern void 	write_cfg_bool		(char *var, bool value);
extern void 	read_cfg_bool		(char *str, bool *val);
extern void 	read_cfg_str		(char *str, char *val);

extern void		option_load			(POSENTRY **arglist);
extern void 	option_save			(void);


extern void		init_default_var	(void);

#endif
