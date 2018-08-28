
/*

    88        88         88 88            
    88        88   ,d    "" 88            
    88        88   88       88            
    88        88 MM88MMM 88 88 ,adPPYba,  
    88        88   88    88 88 I8[    ""  
    88        88   88    88 88  `"Y8ba,   
    Y8a.    .a8P   88,   88 88 aa    ]8I  
     `"Y8888Y"'    "Y888 88 88 `"YbbdP"'  

*/

#define INI_SkipWhitespace(%0,%1) while (IS_IN_RANGE(%0[%1], '\0' + 1, ' ' + 1)) ++%1

/*-------------------------------------------------------------------------*//**
 * <param name="callback">The callback destination.</param>
 * <param name="format">The function name format.</param>
 * <param name="tag">The tag destination.</param>
 * <param name="input">The tag source.</param>
 * <param name="callbackFormat">The callback parameter specifiers.</param>
 * <param name="remote">Use "CallRemoteFunction".</param>
 * <returns>
 *  Was the function found?
 * </returns>
 * <remarks>
 *  Gets a callback given a partial function name and a tag name.  Also saves
 *  the tag elsewhere.  This might not work as a separate function - it will
 *  need to be in the function called by the function with the inlines in.
 * </remarks>
 *//*------------------------------------------------------------------------**/

P:D(INI_GetCallback(callback,format,tag,input,callbackFormat,remote));
#define INI_GetCallback(%0,%1,%2,%3,%4,%5) \
	( \
		strcpy((%2), (%3)), \
		Inline_Reset((%0)), \
		format(YSI_g_sCurLine, sizeof (YSI_g_sCurLine), (%1), (%2)), \
		Callback_Get(callback_tag:YSI_g_sCurLine, (%0), (%4), (%5)) \
	)

static stock INI_ReadFile(File:fhnd, parent[32])
{
	new
		bool:doneParent = false,
		bool:handle = true,
		pos = 0,
		rlen;
	// Read from the start of the file.
	fseek(fhnd, 0, seek_start);
	while ((rlen = fread(fhnd, YSI_g_sCurLine)))
	{
		pos += rlen;
		switch (INI_IdentifyLineType(YSI_g_sCurLine, p0s, p0e, p1s, p1e, p2s, p2e))
		{
			case e_INI_LINE_TYPE_INVALID:
			{
				P:W("Invalid line in INI file \"%s\": %s", fname, YSI_g_sCurLine);
			}
			case e_INI_LINE_TYPE_DATALESS:
			{
				// Do nothing.
			}
			case e_INI_LINE_TYPE_TAG:
			{
				if (doneParent)
				{
					// This was only called to recursively deal with parents.
					return;
				}
				// First, check if it is a tag we might care about.
				YSI_g_sCurLine[p0e] = '\0';
				if (YSI_g_sCurLine[p0s] == '@' && YSI_g_sCurLine[p0s + 1] == '@' && (p0e = strfind(YSI_g_sCurLine, "-") + 1))
				{
					P:W("Filtered tags are deprecated - just use specified callbacks");
					p0s = p0e;
				}
				if (parent[0])
				{
					if (strcmp(YSI_g_sCurLine[p0s], parent))
					{
						handle = false;
						continue;
					}
					doneParent = true;
				}
				else switch (INI_HandleTag(YSI_g_sCurLine[p0s]))
				{
					case 0:
					{
						// Don't deal with this tag.
						handle = false;
						continue;
					}
					case 1:
					{
						// Only deal with local values.
						handle = true;
						goto INI_ReadFile_cont;
					}
				}
				// This parent may have its own parent.
				if (p1s != p1e)
				{
					// Is there a parent tag to deal with?
					YSI_g_sCurLine[p1e] = '\0',
					strcpy(parent, YSI_g_sCurLine[p1s]);
					// Do local and parent values.
					INI_ReadFile_(fhnd, parent),
					fseek(fhnd, pos, seek_start);
				}
				handle = true;
			}
			case e_INI_LINE_TYPE_CONT:
			{
				// Do nothing, wait for the rest of the line.
				if (!handle)
				{
					continue;
				}
			}
			case e_INI_LINE_TYPE_ENTRY:
			{
				if (handle)
				{
					if (YSI_g_sExtendedLine)
					{
						INI_HandleEntry(YSI_g_sExtendedName, Malloc_Get(YSI_g_sExtendedLine, 1));
						free(YSI_g_sExtendedLine),
						YSI_g_sExtendedLine = NO_ALLOC,
						YSI_g_sExtendedName[0] = '\0';
					}
					else if (handle)
					{
						YSI_g_sCurLine[p0e] = YSI_g_sCurLine[p1e] = '\0';
						INI_HandleEntry(YSI_g_sCurLine[p0s], YSI_g_sCurLine[p1s]);
					}
				}
				else
				{
					if (YSI_g_sExtendedLine)
					{
						free(YSI_g_sExtendedLine),
						YSI_g_sExtendedLine = NO_ALLOC,
						YSI_g_sExtendedName[0] = '\0';
					}
					continue;
				}
			}
		}
INI_ReadFile_cont:
		if (p2s != p2e)
		{
			YSI_g_sCurLine[p2e] = '\0';
			INI_HandleComment(YSI_g_sCurLine[p2s]);
		}
	}
	INI_HandleEnd();
}

/*-------------------------------------------------------------------------*//**
 * <remarks>
 *  The `bFileFirst` state (first letter) is only used to generate the name of
 *  the callback.  It isn't used to determine what parameters to pass.  Thus for
 *  many functions the states are paired in their `yxxx`/`nxxx` variants.
 * </remarks>
 *//*------------------------------------------------------------------------**/

static stock INI_DefineStates() <y_ini:y_ini_writing_fast, y_ini:y_ini_writing_slow>
{
}

static stock INI_DefineStates() <
	y_ini:y_ini_reading_yyyy, y_ini:y_ini_reading_nyyy,
	y_ini:y_ini_reading_yyyn, y_ini:y_ini_reading_nyyn,
	y_ini:y_ini_reading_yyny, y_ini:y_ini_reading_nyny,
	y_ini:y_ini_reading_yynn, y_ini:y_ini_reading_nynn,
	y_ini:y_ini_reading_ynyy, y_ini:y_ini_reading_nnyy,
	y_ini:y_ini_reading_ynyn, y_ini:y_ini_reading_nnyn,
	y_ini:y_ini_reading_ynny, y_ini:y_ini_reading_nnny,
	y_ini:y_ini_reading_ynnn, y_ini:y_ini_reading_nnnn>
{
}

static stock
	Func:YSI_g_sCB_issss<issss>,
	Func:YSI_g_sCB_isss<isss>,
	Func:YSI_g_sCB_iss<iss>,
	Func:YSI_g_sCB_ssss<ssss>,
	Func:YSI_g_sCB_sss<sss>,
	Func:YSI_g_sCB_ss<ss>;

/*-------------------------------------------------------------------------*//**
 * <param name="src">The source of the data.</param>
 * <param name="key">The key of the data loaded.</param>
 * <param name="value">The data loaded.</param>
 * <remarks>
 *  Reading: Calls the correct type of callback based on earlier settings.
 *  Writing: Writes out the line with the new or old data.
 * </remarks>
 *//*------------------------------------------------------------------------**/

static stock INI_HandleEntry(INI:src, const key[], const value[]) <y_ini:y_ini_reading_yyyy, y_ini:y_ini_reading_nyyy>
{
	#pragma unused src
	@.YSI_g_sCB_issss(YSI_g_sExtra, YSI_g_sFile, YSI_g_sTag, key, value);
}

static stock INI_HandleEntry(INI:src, const key[], const value[]) <y_ini:y_ini_reading_yyyn, y_ini:y_ini_reading_nyyn>
{
	#pragma unused src
	@.YSI_g_sCB_isss(YSI_g_sExtra, YSI_g_sFile, key, value);
}

static stock INI_HandleEntry(INI:src, const key[], const value[]) <y_ini:y_ini_reading_yyny, y_ini:y_ini_reading_nyny>
{
	#pragma unused src
	@.YSI_g_sCB_isss(YSI_g_sExtra, YSI_g_sTag, key, value);
}

static stock INI_HandleEntry(INI:src, const key[], const value[]) <y_ini:y_ini_reading_yynn, y_ini:y_ini_reading_nynn>
{
	#pragma unused src
	@.YSI_g_sCB_iss(YSI_g_sExtra, key, value);
}

static stock INI_HandleEntry(INI:src, const key[], const value[]) <y_ini:y_ini_reading_ynyy, y_ini:y_ini_reading_nnyy>
{
	#pragma unused src
	@.YSI_g_sCB_ssss(YSI_g_sFile, YSI_g_sTag, key, value);
}

static stock INI_HandleEntry(INI:src, const key[], const value[]) <y_ini:y_ini_reading_ynyn, y_ini:y_ini_reading_nnyn>
{
	#pragma unused src
	@.YSI_g_sCB_sss(YSI_g_sFile, key, value);
}

static stock INI_HandleEntry(INI:src, const key[], const value[]) <y_ini:y_ini_reading_ynny, y_ini:y_ini_reading_nnny>
{
	#pragma unused src
	@.YSI_g_sCB_sss(YSI_g_sTag, key, value);
}

static stock INI_HandleEntry(INI:src, const key[], const value[]) <y_ini:y_ini_reading_ynnn, y_ini:y_ini_reading_nnnn>
{
	#pragma unused src
	@.YSI_g_sCB_ss(key, value);
}

static stock INI_HandleEntry(INI:src, const key[], const value[]) <y_ini:y_ini_writing_slow>
{
	// Used when reading.  Checks if the key is being overwritten, and if not
	// writes it out to the new file.
	#pragma unused src, key, value
}

static stock INI_HandleEntry(INI:src, const key[], const value[]) <y_ini:y_ini_writing_fast>
{
	// Nothing in the current tag is being overwritten, just dump this
	// key/value pair straight to disk with no extra checks.
	#pragma unused src, key, value
}

/*-------------------------------------------------------------------------*//**
 * <param name="src">The source of the data.</param>
 * <param name="comment">The comment from the file.</param>
 * <remarks>
 *  Reading: Does nothing.
 *  Writing: Writes the comment to the file.
 * </remarks>
 *//*------------------------------------------------------------------------**/

static stock INI_HandleComment(INI:src, const comment[]) <y_ini:y_ini_writing_fast, y_ini:y_ini_writing_slow>
{
	#pragma unused src, comment
	// Write the comment.
}

static stock INI_HandleComment(INI:src, const comment[]) <>
{
	#pragma unused src, comment
	// All the reading states ignore comments.
}

/*-------------------------------------------------------------------------*//**
 * <param name="src">The source of the data.</param>
 * <param name="tag">The tag from the file.</param>
 * <remarks>
 *  Reading: Does nothing.
 *  Writing: Dumps the remainder of the previous tag and switches to the new
 *    one.  If there's nothing to dump, moves to the fast dump mode.
 * </remarks>
 *//*------------------------------------------------------------------------**/

static stock INI_HandleTag(INI:src, const tag[]) <y_ini:y_ini_writing_fast, y_ini:y_ini_writing_slow>
{
	// Determine if we have new data for the current tag or not.
	#pragma unused src, tag
}

static stock INI_HandleTag(INI:src, const tag[]) <
	y_ini:y_ini_reading_yyyy, y_ini:y_ini_reading_yyyn,
	y_ini:y_ini_reading_yyny, y_ini:y_ini_reading_yynn,
	y_ini:y_ini_reading_ynyy, y_ini:y_ini_reading_ynyn,
	y_ini:y_ini_reading_ynny, y_ini:y_ini_reading_ynnn>
>
{
	// All the `bFileFirst` states.  Format the callback name accordingly.
	#pragma unused src, tag
}

static stock INI_HandleTag(INI:src, const tag[]) <
	y_ini:y_ini_reading_nyyy, y_ini:y_ini_reading_nyyn,
	y_ini:y_ini_reading_nyny, y_ini:y_ini_reading_nynn,
	y_ini:y_ini_reading_nnyy, y_ini:y_ini_reading_nnyn,
	y_ini:y_ini_reading_nnny, y_ini:y_ini_reading_nnnn>
>
{
	// All the NOT `bFileFirst` states.  Format the callback name accordingly.
	#pragma unused src, tag
}

/*

    88b           d88            88                       db        88888888ba  88  
    888b         d888            ""                      d88b       88      "8b 88  
    88`8b       d8'88                                   d8'`8b      88      ,8P 88  
    88 `8b     d8' 88 ,adPPYYba, 88 8b,dPPYba,         d8'  `8b     88aaaaaa8P' 88  
    88  `8b   d8'  88 ""     `Y8 88 88P'   `"8a       d8YaaaaY8b    88""""""'   88  
    88   `8b d8'   88 ,adPPPPP88 88 88       88      d8""""""""8b   88          88  
    88    `888'    88 88,    ,88 88 88       88     d8'        `8b  88          88  
    88     `8'     88 `"8bbdP"Y8 88 88       88    d8'          `8b 88          88  

*/

/*-------------------------------------------------------------------------*//**
 * <param name="filename">The file to load.</param>
 * <param name="remoteFormat">The format string to generate the remote function to
 * pass the data to once loaded.</param>
 * <param name="bFileFirst">The order of the remoteFormat parameters.</param>
 * <param name="bExtra">Send additional data.</param>
 * <param name="extra">Additional data to send.</param>
 * <param name="bLocal">Call local functions instead of global ones.</param>
 * <param name="bPassTag">Pass the tag as an extra parameter not the function
 * name.</param>
 * <param name="bFilter">Apply the tag name filter to all tags or just prefixed
 * ones?</param>
 * <param name="filter">Text to use to search for which tags to load.</param>
 * <remarks>
 *  bFileFirst sets the order and inclusion of the possible remoteFormat
 *  parameters.  If true the format will add the filename first then the 
 *  current tag, if false the order will be reversed.  This can also be used
 *  to exclude one or the other from the function name by setting the required
 *  parameter to be entered first and then only having one %s in the format
 *  sting.  The default order is tag first for languages compatibility.
 *
 *  This function is now EXTENSIVELY documented here:
 *
 *      <a href="http://forum.sa-mp.com/showthread.php?t=485611" />
 *
 * </remarks>
 *//*------------------------------------------------------------------------**/

stock bool:INI_ParseFile(fname[], remoteFormat[], bool:bFileFirst = false, bool:bExtra = false, extra = 0, bool:bLocal = true, bool:bPassTag = false, bool:bPassFile = false)
{
	new
		File:fhnd = fopen(fname, io_read);
	if (!fhnd)
		return false;
	// Read the file.  We have no parent and want to be in read mode.  What we
	// do with the lines is determined by the state and the current line type.
	new parent[32] = "";
	if (bFileFirst)
	{
		if (bExtra)
		{
			if (bPassTag)
			{
				if (bPassFile)
					state y_ini:y_ini_reading_yyyy;
				else
					state y_ini:y_ini_reading_yyyn;
			}
			else
			{
				if (bPassFile)
					state y_ini:y_ini_reading_yyny;
				else
					state y_ini:y_ini_reading_yynn;
			}
		}
		else
		{
			if (bPassTag)
			{
				if (bPassFile)
					state y_ini:y_ini_reading_ynyy;
				else
					state y_ini:y_ini_reading_ynyn;
			}
			else
			{
				if (bPassFile)
					state y_ini:y_ini_reading_ynny;
				else
					state y_ini:y_ini_reading_ynnn;
			}
		}
	}
	else
	{
		if (bExtra)
		{
			if (bPassTag)
			{
				if (bPassFile)
					state y_ini:y_ini_reading_nyyy;
				else
					state y_ini:y_ini_reading_nyyn;
			}
			else
			{
				if (bPassFile)
					state y_ini:y_ini_reading_nyny;
				else
					state y_ini:y_ini_reading_nynn;
			}
		}
		else
		{
			if (bPassTag)
			{
				if (bPassFile)
					state y_ini:y_ini_reading_nnyy;
				else
					state y_ini:y_ini_reading_nnyn;
			}
			else
			{
				if (bPassFile)
					state y_ini:y_ini_reading_nnny;
				else
					state y_ini:y_ini_reading_nnnn;
			}
		}
	}
	INI_ReadFile(fhnd, parent);
}

