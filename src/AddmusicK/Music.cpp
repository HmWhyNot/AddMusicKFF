#include "Music.h"
#include "Utility.h"

#include <cmath>

using namespace AddMusic;

constexpr int tmpTrans[19] 			{ 0, 0, 5, 0, 0, 0, 0, 0, 0, -5, 6, 0, -5, 0, 0, 8, 0, 0, 0 };
constexpr int instrToSample[30] 	{ 0x00, 0x01, 0x02, 0x03, 0x04, 0x07, 0x08, 0x09, 0x05, 0x0A,	// \ Instruments
0x0B, 0x01, 0x10, 0x0C, 0x0D, 0x12, 0x0C, 0x11, 0x01,		// /
0x00, 0x00,							// Nothing
0x0F, 0x06, 0x06, 0x0E, 0x0E, 0x0B, 0x0B, 0x0B, 0x0E };		// Percussion

constexpr int hexLengths[] 			{ 2, 2, 3, 4, 4, 1,
2, 3, 2, 3, 2, 4, 2, 2, 3, 4, 2, 4, 4, 3, 2, 4,
1, 4, 4, 3, 2, 9, 3, 4, 2, 3, 3, 2, 5, 1, 1 };

/*
 * TODO: Initialization:
for (int z = 0; z < 19; z++)
	transposeMap[z] = tmpTrans[z];
for (int z = 0; z < 16; z++)	// Add some spaces to the end.
	text += ' ';
*/

void Music::compile()
{
	pos = 0;
	while (pos < text.length())
	{
#ifdef _DEBUG
		current = text.substr(pos);
#endif

		doReplacement();

		if (hexLeft != 0 && !isspace(tolower(text[pos])) && tolower(text[pos]) != '$' && text[pos] != '\n')
		{
			if (currentHex == 0xE6 && songTargetProgram == 1)
			{
				data[channel][data[channel].size() - 1] = 0xFD;
				hexLeft = 0;
			}
			else
			{
				throw AddmusicException("Unknown hex command.");
			}
		}

		switch (tolower(text[pos]))
		{
		case '?': parseQMarkDirective();	break;
			//case '!': parseExMarkDirective();	break;
		case '#': parseChannelDirective();	break;
		case 'l': parseLDirective();		break;
		case 'w': parseGlobalVolumeCommand();	break;
		case 'v': parseVolumeCommand();		break;
		case 'q': parseQuantizationCommand();	break;
		case 'y': parsePanCommand();		break;
		case '/': parseIntroDirective();	break;
		case 't': parseT();			break;
		case 'o': parseOctaveDirective();	break;
		case '@': parseInstrumentCommand();	break;
		case '(': parseOpenParenCommand();	break;
		case '[': parseLoopCommand();		break;
		case ']': parseLoopEndCommand();	break;
		case '*': parseStarLoopCommand();	break;
		case 'p': parseVibratoCommand();	break;
		case '{': parseTripletOpenDirective();	break;
		case '}': parseTripletCloseDirective();	break;
		case '>': parseRaiseOctaveDirective();	break;
		case '<': parseLowerOctaveDirective();	break;
		case '&': parsePitchSlideCommand();	break;
		case '$': parseHexCommand();		break;
		case 'h': parseHDirective();		break;
		case 'n': parseNCommand();		break;
		case '\"':parseReplacementDirective();	break;
		case '\n': pos++; line++;		break;
		case '|': pos++; hexLeft = 0;		break;
		case 'c': case 'd': case 'e': case 'f': case 'g': case 'a': case 'b': case 'r': case '^':
			parseNote();			break;
		case ';':
			parseComment();			break;		// Needed for comments in quotes
		default:
			if (isspace(text[pos]))
			{
				pos++; break;
			}
			else
			{
				throw AddmusicException(std::string("Unexpected character \"") + text[pos] + "\" found.", true, this);
				pos++; break;
			}
		}
	}

	pointersFirstPass();
}

void Music::parseComment()
{
	if (songTargetProgram == 2)
	{

		pos++;
		while (pos < text.length())
		{
			if (text[pos] == '\n')
				break;
			pos++;
		}
		line++;
	}
	else
	{
		pos++;
		throw AddmusicException("Illegal use of comments. Sorry about that. Should be fixed in AddmusicK 2.", false, this);
	}
}

void Music::printChannelDataNonVerbose(int totalSize)
{
	int n = 60 - printf("%s: ", name.c_str());
	for (int i = 0; i < n; i++)
		putchar('.');
	putchar(' ');

	if (knowsLength)
	{
		int s = (unsigned int)std::floor((mainLength + introLength) / (2.0 * tempo) + 0.5);
		printf("%d:%02d, 0x%04X bytes\n", (int)(std::floor((introSeconds + mainSeconds) / 60) + 0.5), (int)(std::floor(introSeconds + mainSeconds) + 0.5) % 60, totalSize);
	}
	else
	{
		printf("?:??, 0x%04X bytes\n", totalSize);
	}


}

void Music::parseQMarkDirective()
{
	pos++;
	i = getInt();
	i = (i == -1) ? 0 : i;
	switch (i)
	{
		case 0: doesntLoop = true; break;
		case 1: noMusic[channel][0] = true; break;
		case 2: noMusic[channel][1] = true; break;
	}
}

void Music::parseExMarkDirective()
{
	pos = -1;
}

void Music::parseChannelDirective()
{
	pos++;
	if (isalpha(text[pos]))
	{
		parseSpecialDirective();
		return;
	}

	i = getInt();
	if (i == -1)
		throw AddmusicException("Error parsing channel directive.", false, this);
	if (i < 0 || i > 7)
		throw AddmusicException("Illegal value for channel directive.", false, this);

		channel = i;
	q[8] = q[channel];
	updateQ[8] = updateQ[channel];
	prevNoteLength = -1;

	hTranspose = 0;
	usingHTranspose = false;
	channelDefined = true;
}

void Music::parseLDirective()
{
	pos++;
	i = getInt();
	if (i == -1 && text[pos] == '=' && targetAMKVersion >= 4)
	{
		pos++;
		i = getInt();

		if (i == -1)
		{
			throw AddmusicException("Error parsing \"l\" directive.", false, this);
		}
		defaultNoteLength = i;
	}
	else if (i == -1)
		throw AddmusicException("Error parsing \"l\" directive.", false, this);
	else if (i < 1 || i > 192)
		throw AddmusicException("Illegal value for \"l\" directive.", false, this);
	else {
		if (192 % i != 0 && fractionNoteLengthWarning) {
			throw AddmusicException("WARNING: A default note length was used that is not divisible by 192 ticks, and thus results in a fractional tick value.", false, this);
			fractionNoteLengthWarning = false;
		}
		defaultNoteLength = 192 / i;
	}
	if (targetAMKVersion >= 4) {
		defaultNoteLength = getNoteLengthModifier(defaultNoteLength, false);
	}
}

void Music::parseGlobalVolumeCommand()
{
	int duration = -1;
	int volume = -1;

	pos++;
	volume = getInt();
	if (volume == -1)
		throw AddmusicException("Error parsing global volume (\"w\") command.", false, this);

	if (targetAMKVersion >= 3) {
		skipSpaces();
		if (text[pos] == ',')
			{
				pos++;
				skipSpaces();
		
				duration = volume;
		
				volume = getInt();
				if (volume == -1) 
					throw AddmusicException("Error parsing global volume (\"w\") command.", false, this);
			}
	}
	if (volume < 0 || volume > 255) 
		throw AddmusicException("Illegal value for global volume (\"w\") command.", false, this);

	if (duration == -1) {
		append(0xE0);
		append(volume);
	}
	else {
		if (duration < 0 || duration > 255)
			throw AddmusicException("Illegal value for global volume (\"w\") command.", false, this);
		append(0xE1);
		append(divideByTempoRatio(duration, false));
		append(volume);
	}
}
void Music::parseVolumeCommand()
{
	int duration = -1;
	int volume = -1;

	pos++;
	volume = getInt();
	if (volume == -1)
		throw AddmusicException("Error parsing volume (\"v\") command.", false, this);

	if (targetAMKVersion >= 3) {
		skipSpaces();
		if (text[pos] == ',')
			{
				pos++;
				skipSpaces();
		
				duration = volume;
		
				volume = getInt();
				if (volume == -1)
					throw AddmusicException("Error parsing volume (\"v\") command.", false, this);
			}
	}
	if (volume < 0 || volume > 255)
		throw AddmusicException("Illegal value for volume (\"v\") command.", false, this);

	if (duration == -1) {
		append(0xE7);
		append(volume);
	}
	else {
		if (duration < 0 || duration > 255)
			throw AddmusicException("Illegal value for volume (\"v\") command.", false, this);
		append(0xE8);
		append(divideByTempoRatio(duration, false));
		append(volume);
	}
}
void Music::parseQuantizationCommand()
{
	pos++;
	i = getHex();
	if (i == -1)
		throw AddmusicException("Error parsing quantization (\"q\") command.", false, this);
	if (i < 1 || i > 0x7F)
		throw AddmusicException("Error parsing quantization (\"q\") command.", false, this);

	if (channel == 8)
	{
		q[prevChannel] = i;
		updateQ[prevChannel] = true;
	}
	else
	{
		q[channel] = i;
		updateQ[channel] = true;
	}

	q[8] = i;
	updateQ[8] = true;
}
void Music::parsePanCommand()
{
	pos++;
	i = getInt();
	int pan = i;

	if (i == -1) throw AddmusicException("Error parsing pan (\"y\") command.", false, this);
	if (i < 0 || i > 20) throw AddmusicException("Illegal value for pan (\"y\") command.", false, this);

		skipSpaces();

	if (text[pos] == ',')
	{
		pos++;
		i = getInt();
		if (i == -1) throw AddmusicException("Error parsing pan (\"y\") command.", false, this);
		if (i > 2)  throw AddmusicException("Illegal value for pan (\"y\") command.", false, this);
			pan |= (i << 7);
		skipSpaces();
		if (text[pos] != ',') throw AddmusicException("Error parsing pan (\"y\") command.", false, this);

			pos++;
		i = getInt();
		if (i == -1) throw AddmusicException("Error parsing pan (\"y\") command.", false, this);
		if (i > 2)  throw AddmusicException("Illegal value for pan (\"y\") command.", false, this);
			pan |= (i << 6);
	}

	append(0xDB);
	append(pan);
}
void Music::parseIntroDirective()
{
	if (channel == 8) throw AddmusicException("Intro directive found within a loop.", false, this);

	if (hasIntro == false)
		tempoChanges.push_back(std::pair<double, int>(channelLengths[channel], -((int)tempo)));
	else
	{
		for (size_t z = 0; z < tempoChanges.size(); z++)
		{
			if (tempoChanges[z].second < 0)
			{
				tempoChanges[z].second = -((int)tempo);
			}
		}
	}

	hasIntro = true;
	pos++;
	phrasePointers[channel][1] = data[channel].size();
	prevNoteLength = -1;
	passedIntro[channel] = true;
	introLength = channelLengths[channel];
}
void Music::parseT()
{
	pos++;
	if (strncmp(&text[pos], "uning[", 6) == 0)
		parseTransposeDirective();
	else
		parseTempoCommand();
}
void Music::parseTempoCommand()
{
	int duration = -1;
	int ltempo = -1;

	ltempo = getInt();
	if (ltempo == -1) throw AddmusicException("Error parsing tempo (\"t\") command.", false, this);

	if (targetAMKVersion >= 3) {
		skipSpaces();
		if (text[pos] == ',')
			{
				pos++;
				skipSpaces();
		
				duration = ltempo;
		
				ltempo = getInt();
				if (ltempo == -1) throw AddmusicException("Error parsing tempo (\"t\") command.", false, this);
			}
	}
	
	if (ltempo < 0 || ltempo > 255) throw AddmusicException("Illegal value for tempo (\"t\") command.", false, this);
	
	tempo = divideByTempoRatio(ltempo, false);
	
	if (tempo == 0) {
		throw AddmusicException("Tempo has been zeroed out by #halvetempo", false, this);
		tempo = ltempo;
	}

	if (duration == -1) {
		tempoDefined = true;

		if (channel == 8 || inE6Loop)								// Not even going to try to figure out tempo changes inside loops.  Maybe in the future.
		{
			guessLength = false;
		}
		else
		{
			tempoChanges.push_back(std::pair<double, int>(channelLengths[channel], tempo));
		}

		append(0xE2);
		append(tempo);
	}
	else {
		if (duration < 0 || duration > 255) throw AddmusicException("Illegal value for tempo (\"t\") command.", false, this);
		guessLength = false;		// NOPE.  Nope nope nope nope nope nope nope nope nope nope.
		append(0xE3);
		append(divideByTempoRatio(duration, false));
		append(tempo);
	}
}

void Music::parseTransposeDirective()
{
	pos += 6;

	i = getInt();

	if (i == -1) throw AddmusicException("Error parsing tuning directive.", false, this);
	if (i < 0 || i > 255)  throw AddmusicException("Illegal instrument value for tuning directive.", false, this);

	if (text[pos] != ']') throw AddmusicException("Error parsing tuning directive.", false, this);
		pos++;
	skipSpaces();

	if (text[pos] != '=') throw AddmusicException("Error parsing tuning directive.", false, this);
		pos++;

	while (true)
	{
		skipSpaces();

		bool plus = true;
		if (text[pos] == '+') pos++;
		else if (text[pos] == '-') { pos++; plus = false; }

		j = getInt();

		if (j == -1) throw AddmusicException("Error parsing tuning directive.", false, this);

		if (plus == false) j = -j;
		transposeMap[i] = j;

		skipSpaces();

		if (text[pos] != ',') break;
		pos++;
		i++;
		if (i >= 256) throw AddmusicException("Illegal value for tuning directive.", false, this);
	}
}

void Music::parseOctaveDirective()
{
	pos++;
	i = getInt();
	if (i == -1) throw AddmusicException("Error parsing octave (\"o\") directive.", false, this);
	if (i < 0 || i > 6) throw AddmusicException("Error parsing octave (\"o\") directive.", false, this);
		octave = i;
}
void Music::parseInstrumentCommand()
{
	pos++;
	bool direct = false;
	if (text[pos] == '@')
	{
		pos++;
		direct = true;
	}

	i = getInt();
	if (i == -1) throw AddmusicException("Error parsing instrument (\"@\") command.", false, this);
	if (i < 0 || i > 255) throw AddmusicException("Illegal value for instrument (\"@\") command.", false, this);

	if ((i <= 18 || direct) || i >= 30)
	{
		if (convert)
		{
			if (i >= 0x13 && i < 30)	// Change it to an HFD custom instrument.
				i = i - 0x13 + 30;
		}
		if (optimizeSampleUsage)
		{
			if (i < 30)
				usedSamples[instrToSample[i]] = true;
			else if ((i - 30) * 6 < instrumentData.size())
				usedSamples[instrumentData[(i - 30) * 6]] = true;
			else
				throw AddmusicException("This custom instrument has not been defined yet.", false, this);
		}

		if (songTargetProgram == 1)
		{
			ignoreTuning[channel] = false;
		}

		append(0xDA);
		append(i);
	}

	if (i < 30)
		if (optimizeSampleUsage)
			usedSamples[instrToSample[i]] = true;

	instrument[channel] = i;
	//if (htranspose[i] == true)
	if (songTargetProgram == 2 && i < 19) {
		hTranspose = 0;
		usingHTranspose = false;
		transposeMap[instrument[channel]] = ::tmpTrans[instrument[channel]];
	}
}

void Music::parseOpenParenCommand()
{
	if (text[pos + 1] == '"' || text[pos + 1] == '@')
		parseSampleLoadCommand();
	else
		parseLabelLoopCommand();
}

void Music::parseSampleLoadCommand()
{
	pos += 1;
	if (text[pos] == '@')
	{
		pos++;
		i = getInt();
		i = instrToSample[i];
		if (text[pos] != ',')
		{
			throw AddmusicException("Error parsing sample load command.", false, this);
				return;
		}
		pos++;
	}
	else
	{
		pos++;
		std::string s = "";
		while (text[pos] != '"')
		{
			if (pos >= text.length()) throw AddmusicException("Error parsing sample load command.", false, this);
				s += text[pos];
			pos++;
		}
		pos++;
		if (text[pos] != ',')
		{
			throw AddmusicException("Error parsing sample load command.", false, this);
				return;
		}
		i = -1;

		s = basepath + s;

		int gs = getSample(s, this);
		for (j = 0; j < mySamples.size(); j++)
		{
			if (mySamples[j] == gs)
			{
				i = j;
				break;
			}
		}

		if (i == -1)
			throw AddmusicException("The specified sample was not included in this song.", false, this);

		pos++;
	}
	skipSpaces();
	if (text[pos] != '$')
	{
		throw AddmusicException("Error parsing sample load command.", false, this);
			return;
	}
	pos++;
	j = getHex();
	if (j == -1 || j > 0xFF)
		throw AddmusicException("Error parsing sample load command.", false, this);

	if (text[pos] != ')')
	{
		throw AddmusicException("Error parsing sample load command.", false, this);
			return;
	}
	pos++;

	if (optimizeSampleUsage)
		usedSamples[i] = true;

	append(0xF3);
	append(i);
	append(j);


}

void Music::parseLabelLoopCommand()
{

	pos++;
	if (text[pos] == '!')
	{
		if (targetAMKVersion < 2)
			throw AddmusicException("Unrecognized character '!'.", false, this);

		pos++;
		skipSpaces();

		if (channelDefined == true)						// A channel's been defined, we're parsing a remote
		{
			if (targetAMKVersion >= 3 && text[pos] == '!')			//if it was actually !! instead of just !
			{
				pos++;
				//--------------------------------------
				// Reset RemoteCommand
				//--------------------------------------
				try
				{
					i = getIntWithNegative();
				}
				catch (...)
				{
					throw AddmusicException("Error parsing remote code reset. Remember that remote code cannot be defined within a channel.", false, this);
				}
				skipSpaces();

				if (text[pos] != ')')
					throw AddmusicException("Error parsing remote reset.", false, this);
					pos++;

				switch (i)
				{
				case 0:
					append(0xFC);
					append(0x00);
					append(0x00);
					append(0x00);
					append(0x00);
					break;
				case -1:
					append(0xFC);
					append(0x00);
					append(0x00);
					append(0x08);
					append(0x00);
					break;
				default:
					append(0xFC);
					append(0x00);
					append(0x00);
					append(0x07);
					append(0x00);
					break;
				}
				return;
			}
			//--------------------------------------
			// Call RemoteCommand
			//--------------------------------------
			//bool negative = false;
			i = getInt();
			if (i == -1) throw AddmusicException("Error parsing remote code setup.", false, this);
				skipSpaces();
			if (text[pos] != ',') throw AddmusicException("Error parsing code setup.", false, this);
				pos++;
			skipSpaces();
			//if (text[pos] == '-') negative = true, pos++;
			try
			{
				j = getIntWithNegative();
			}
			catch (...)
			{
				throw AddmusicException("Error parsing remote code setup. Remember that remote code cannot be defined within a channel.", false, this);
			}
			//if (negative) j = -j;
			skipSpaces();
			int k = 0;
			if (j == 1 || j == 2)
			{
				if (text[pos] != ',') throw AddmusicException("Error parsing remote code setup. Missing the third argument.", false, this);
					pos++;
				skipSpaces();
				if (text[pos] == '$')
				{
					pos++;
					k = getHex();
					if (k == -1) throw AddmusicException("Error parsing remote code setup. Could not parse the third argument as a hex command.", false, this);
				}
				else
				{
					k = getNoteLength(getInt());
					if (k > 0x100)
						throw AddmusicException("Note length specified was too large.", false, this);
					else if (k == 0x100)
					k = 0;
				}

				skipSpaces();
			}

			if (text[pos] != ')')
				throw AddmusicException("Error parsing remote setup.", false, this);
				pos++;

			if (text[pos] == '[')
			{
				throw AddmusicException("Remote code cannot be defined within a channel.", false, this);
			}
			append(0xFC);
			loopLocations[channel].push_back(data[channel].size());

			append(loopPointers[i] & 0xFF);
			append(loopPointers[i] >> 8);
			append(j);
			append(k);
			return;
		}
		else								// We're outside of a channel, this is a remote call definition.
		{
			i = getInt();
			if (i == -1)
				throw AddmusicException("Error parsing remote code definition.", false, this);

			skipSpaces();

			if (text[pos] != ')')
				throw AddmusicException("Error parsing remote code definition.", false, this);

			pos++;

			if (text[pos] == '[')
			{
				loopLabel = i;
				remoteDefinitionType = j;
				inRemoteDefinition = true;
				return;
			}
			else
			{
				throw AddmusicException("Error parsing remote code definition; the definition was missing.", false, this);
			}
			return;

		}
	}
	if (channel == 8) throw AddmusicException("Nested loops are not allowed.", false, this);
		i = getInt();
	if (i == -1) throw AddmusicException("Error parsing label loop.", false, this);
		i++;						// Needed to allow for songs that used label 0.
	if (i == 0) throw AddmusicException("Illegal value for loop label.", false, this);
	if (i >= 0x10000)  throw AddmusicException("Illegal value for loop label.", false, this);

	if (text[pos] != ')')  throw AddmusicException("Error parsing label loop.", false, this);

		pos++;

	updateQ[channel] = true;
	updateQ[8] = true;
	prevNoteLength = -1;

	if (text[pos] == '[')				// If this is a loop definition...
	{
		loopLabel = i;				// Just set the loop label to this. The rest of the code is handled in the respective function.
	}
	else						// Otherwise, if this is a loop call...
	{
		loopLabel = i;

		prevNoteLength = -1;

		if ((unsigned short)loopPointers[loopLabel] == (unsigned short)-1) throw AddmusicException("Label not yet defined.", false, this);
		j = getInt();
		if (j == -1) j = 1;
		if (j < 1 || j > 255)
		{
			throw AddmusicException("Invalid loop count.", false, this);
				j = 1;
		}

		handleNormalLoopRemoteCall(j);

		append(0xE9);
		loopLocations[channel].push_back(data[channel].size());

		append(loopPointers[i] & 0xFF);
		append(loopPointers[i] >> 8);
		append(j);

		loopLabel = 0;
	}

}
void Music::parseLoopCommand()
{
	pos++;
	if (channel < 8)
		updateQ[channel] = true;
	updateQ[8] = true;
	prevNoteLength = -1;

	if (text[pos] == '[')			// This is an $E6 loop.
	{
		pos++;

		if (text[pos] == '[')
			throw AddmusicException("An ambiguous use of the [ and [[ loop delimiters was found (\"[[[\").  Separate\nthe \"[[\" and \"[\" to clarify your intention.", true, this);

		if (inE6Loop == true)
			throw AddmusicException("You cannot nest a subloop within another subloop.", false, this);
		if (loopLabel > 0 && text[pos - 2] == ')')
			throw AddmusicException("A label loop cannot define a subloop.  Use a standard or remote loop instead.", false, this);


			handleSuperLoopEnter();

		append(0xE6);
		append(0x00);
		return;
	}
	else if (channel == 8)
	{
		throw AddmusicException("You cannot nest standard [ ] loops.", false, this);
	}


	prevLoop = data[8].size();


	prevChannel = channel;				// We're in a loop now, which is represented as channel 8.
	channel = 8;					// So we have to back up the current channel.
	prevNoteLength = -1;
	instrument[8] = instrument[prevChannel];
	if (songTargetProgram == 1) ignoreTuning[8] = ignoreTuning[prevChannel];		// More AM4 tuning stuff.  Related to the line above it.

	if (loopLabel > 0)
	{
		if ((signed short)loopPointers[loopLabel] != -1)
		{
			throw AddmusicException("Label redefinition.", false, this);
		}
	}

	if (loopLabel > 0)
		loopPointers[loopLabel] = prevLoop;

	handleNormalLoopEnter();

}

void Music::parseLoopEndCommand()
{
	pos++;
	if (channel < 8)
		updateQ[channel] = true;

	updateQ[8] = true;
	prevNoteLength = -1;
	if (text[pos] == ']')
	{
		pos++;
		if (text[pos] == ']')
			throw AddmusicException("An ambiguous use of the ] and ]] loop delimiters was found (\"]]]\").  Separate\nthe \"]]\" and \"]\" to clarify your intention.", true, this);

			i = getInt();

		if (i == 1)
			throw AddmusicException("A subloop cannot only repeat once. It's pointless anyway.", false, this);

		if (inE6Loop == false)
			throw AddmusicException("A subloop end was found outside of a subloop.", false, this);
		if (i == -1)
			throw AddmusicException("Error parsing subloop command; the loop count was missing.", false, this);
			//if (i == 0)
			//	throw AddmusicException("A subloop cannot loop 0 times.", false, this);

			inE6Loop = false;

		handleSuperLoopExit(i);

		append(0xE6);
		append(i - 1);
		return;
	}
	else if (channel != 8) throw AddmusicException("Loop end found outside of a loop.", false, this);
	i = getInt();
	if (i != -1 && inRemoteDefinition)
		throw AddmusicException("Remote code definitions cannot repeat.", false, this);

	if (i == -1) i = 1;

	if (i < 1 || i > 255) throw AddmusicException("Invalid loop count.", false, this);

	append(0);
	channel = prevChannel;

	handleNormalLoopExit(i);

	if (!inRemoteDefinition)
	{
		append(0xE9);
		loopLocations[channel].push_back(data[channel].size());
		append(prevLoop & 0xFF);
		append(prevLoop >> 8);
		append(i);
	}
	inRemoteDefinition = false;
	loopLabel = 0;
}
void Music::parseStarLoopCommand()
{

	pos++;

	if (channel == 8)  throw AddmusicException("Nested loops are not allowed.", false, this);

		updateQ[channel] = true;
	updateQ[8] = true;
	prevNoteLength = -1;

	i = getInt();
	if (i == -1) i = 1;
	if (i < 1 || i > 255)
	{
		throw AddmusicException("Invalid loop count.", false, this);
		i = 1;
	}

	handleNormalLoopRemoteCall(i);

	append(0xE9);
	loopLocations[channel].push_back(data[channel].size());
	append(prevLoop & 0xFF);
	append(prevLoop >> 8);
	append(i);
}
void Music::parseVibratoCommand()
{
	pos++;
	int t1, t2, t3;
	t1 = getInt();
	if (t1 == -1) throw AddmusicException("Error parsing vibrato command.", false, this);
	skipSpaces();
	if (text[pos] != ',') throw AddmusicException("Error parsing vibrato command.", false, this);
	pos++;
	skipSpaces();
	t2 = getInt();
	if (t2 == -1) throw AddmusicException("Error parsing vibrato command.", false, this);
	skipSpaces();

	if (text[pos] == ',')	// The user has specified the delay.
	{
		pos++;
		skipSpaces();
		t3 = getInt();
		if (t3 == -1) throw AddmusicException("Error parsing vibrato command.", false, this);
		if (t1 < 0 || t1 > 255) throw AddmusicException("Illegal value for vibrato delay.", false, this);
		if (t2 < 0 || t2 > 255) throw AddmusicException("Illegal value for vibrato rate.", false, this);
		if (t3 < 0 || t3 > 255) throw AddmusicException("Illegal value for vibrato extent.", false, this);

		append(0xDE);
		append(divideByTempoRatio(t1, false));
		append(multiplyByTempoRatio(t2));
		append(t3);
	}
	else			// The user has not specified the delay.
	{
		if (t1 < 0 || t1 > 255) throw AddmusicException("Illegal value for vibrato rate.", false, this);
		if (t2 < 0 || t2 > 255) throw AddmusicException("Illegal value for vibrato extent.", false, this);

		append(0xDE);
		append(00);
		append(multiplyByTempoRatio(t1));
		append(t2);
	}

}
void Music::parseTripletOpenDirective()
{
	pos++;
	if (triplet == false)
		triplet = true;
	else
		throw AddmusicException("Triplet on directive found within a triplet block.", false, this);
}
void Music::parseTripletCloseDirective()
{
	pos++;
	if (triplet == true)
		triplet = false;
	else
		throw AddmusicException("Triplet off directive found outside of a triplet block.", false, this);
}
void Music::parseRaiseOctaveDirective()
{
	pos++;
	octave++;
	if (octave > 7)
	{
		octave = 7;
		throw AddmusicException("The octave has been raised too high.", false, this);
	}
}
void Music::parseLowerOctaveDirective()
{
	pos++;
	octave--;
	if (octave < -1)
	{
		octave = 0;
		throw AddmusicException("The octave has been dropped too low.", false, this);
	}
}
void Music::parsePitchSlideCommand()
{
	pos++;
	if (inPitchSlide)
	{
		throw AddmusicException("Pitch slide directive specified multiple times in a row.", false, this);
		return;
	}
	inPitchSlide = true;
}
void Music::parseHFDInstrumentHack(int addr, int bytes)
{
	int byteNum = 0;
	do
	{
		skipSpaces();
		if (text[pos] != '$')
		{
			throw AddmusicException("Unknown HFD hex command.", false, this);
			return;
		}
		pos++;
		i = getHex();
		if (i == -1 || i > 0xFF)
		{
			throw AddmusicException("Unknown HFD hex command.", false, this);
			return;
		}
		instrumentData.push_back(i);
		bytes--;
		byteNum++;
		addr++;
		if (byteNum == 1)
		{
			if (optimizeSampleUsage)
				usedSamples[i] = true;
		}
		if (byteNum == 5)
		{
			instrumentData.push_back(0);	// On the 5th uint8_t, we need to add a 0 as the new sub-multiplier.
			byteNum = 0;
		}

	} while (bytes >= 0);
}

void Music::parseHFDHex()
{
	skipSpaces();
	if (text[pos] == '$')
	{
		pos++;
		i = getHex();
		if (i == -1)
		{
			throw AddmusicException("Error parsing hex command.", false, this);
			return;
		}

		if (i == 0x80 && convert)
		{
			int reg;
			int val;
			skipSpaces();
			if (text[pos] != '$')
			{
				throw AddmusicException("Unknown HFD hex command.", false, this);
				return;
			}
			pos++;
			reg = getHex();
			if (reg == -1)
			{
				throw AddmusicException("Unknown HFD hex command.", false, this);
				return;
			}
			skipSpaces();
			if (text[pos] != '$')
			{
				throw AddmusicException("Unknown HFD hex command.", false, this);
				return;
			}
			pos++;
			val = getHex();
			if (val == -1)
			{
				throw AddmusicException("Unknown HFD hex command.", false, this);
				return;
			}

			if (!(reg == 0x6D || reg == 0x7D))	// Do not write the HFD header hex bytes.
			{
				if (reg == 0x6C)			// Noise command gets special treatment.
				{
					append(0xF8);
					append(val);
				}
				else
				{
					append(0xF6);
					append(reg);
					append(val);
				}
			}
			else
			{
				songTargetProgram = 1;		// The HFD header bytes indicate this as being an AM4 song, so it gets AM4 treatment.
			}
			hexLeft = 0;
		}
		else if (i == 0x81 && convert)
		{
			skipSpaces();
			if (text[pos] != '$')
			{
				throw AddmusicException("Unknown HFD hex command.", false, this);
				return;
			}
			pos++;
			i = getHex();
			if (i == -1)
			{
				throw AddmusicException("Unknown HFD hex command.", false, this);
				return;
			}

			append(0xFA);
			append(0x02);
			append(i);
			hexLeft = 0;
		}
		else if ((i == 0x83) && convert)
		{
			throw AddmusicException("Unknown HFD hex command.", false, this);
			return;
		}
		else if (i == 0x82 && convert)
		{
			int addr;
			int bytes;
			skipSpaces();
			if (text[pos] != '$')
			{
				throw AddmusicException("Unknown HFD hex command.", false, this);
				return;
			}
			pos++;
			i = getHex();
			if (i == -1)
			{
				throw AddmusicException("Unknown HFD hex command.", false, this);
				return;
			}
			addr = i << 8;

			skipSpaces();
			if (text[pos] != '$')
			{
				throw AddmusicException("Unknown HFD hex command.", false, this);
				return;
			}
			pos++;
			i = getHex();
			if (i == -1)
			{
				throw AddmusicException("Unknown HFD hex command.", false, this);
				return;
			}
			addr |= i;

			skipSpaces();
			if (text[pos] != '$')
			{
				throw AddmusicException("Unknown HFD hex command.", false, this);
				return;
			}
			pos++;
			i = getHex();
			if (i == -1)
			{
				throw AddmusicException("Unknown HFD hex command.", false, this);
				return;
			}
			bytes = i << 8;

			skipSpaces();
			if (text[pos] != '$')
			{
				throw AddmusicException("Unknown HFD hex command.", false, this);
				return;
			}
			pos++;
			i = getHex();
			if (i == -1)
			{
				throw AddmusicException("Unknown HFD hex command.", false, this);
				return;
			}
			bytes |= i;

			if (addr == 0x6136)
			{
				parseHFDInstrumentHack(addr, bytes);
				return;
			}
			do
			{
				skipSpaces();
				if (text[pos] != '$')
				{
					throw AddmusicException("Unknown HFD hex command.", false, this);
					return;
				}
				pos++;
				i = getHex();
				if (i == -1)
				{
					throw AddmusicException("Unknown HFD hex command.", false, this);
					return;
				}
				//append(0xF7);
				//append(i);
				//append(addr >> 16);
				//append(addr & 0xFF);	//Don't do this stuff; we won't know what we're overwriting.
				bytes--;
				addr++;

			} while (bytes >= 0);
			hexLeft = 0;

		}
		else
		{
			currentHex = 0xED;
			hexLeft = hexLengths[currentHex - 0xDA] - 1 - 1;
			append(0xED);
			if (convert)
				append(i);
			else
				append(i);
		}
	}
	else
	{
		throw AddmusicException("Unknown HFD hex command.", false, this);
	}
}
//static bool validateTremolo;

static bool nextNoteIsForDD;

void Music::parseHexCommand()
{
	pos++;
	i = getHex();
	if (i == -1 || i > 0xFF)
	{
		throw AddmusicException("Error parsing hex command.", false, this);
		return;
	}

	if (validateHex)
	{
		if (hexLeft == 0)
		{
			//validateTremolo = true;
			currentHex = i;
			if (i > 0xF2 && songTargetProgram == 1 && nonNativeHexWarning) {
				throw AddmusicException("WARNING: A hex command was used which is not native to AddMusic405.\nDid you mean: #amm", false, this);
				nonNativeHexWarning = false;
			}
			if (i > 0xFA && songTargetProgram == 2 && nonNativeHexWarning) {
				throw AddmusicException("WARNING: A hex command was used which is not native to AddMusicM.\nDid you mean: #amk 1", false, this);
				nonNativeHexWarning = false;
			}
			if (i < 0xDA)
			{
				if (manualNoteWarning)
				{
					if (targetAMKVersion == 0)
					{
						throw AddmusicException("Warning: A hex command was found that will act as a note instead of a special\neffect. If this is a song you're using from someone else, you can most likely\nignore this message, though it may indicate that a necessary #amm or #am4 is\nmissing.", false, this);
						manualNoteWarning = false;
					}
					else
					{
						throw AddmusicException("Unknown hex command.", false, this);
					}
				}
			}
			else if (i > 0xFE)
			{
				throw AddmusicException("Unknown hex command.", false, this);
			}
			else if (i == 0xED && songTargetProgram == 1)
			{
				parseHFDHex();
				return;
			}
			else if (i == 0xFB)
			{
				skipSpaces();
				if (text[pos] != '$')
					throw AddmusicException("Unknown hex command.", false, this);
					pos++;
				j = getHex();
				if (j == -1 || i > 0xFF)
					throw AddmusicException("Error parsing hex command.", false, this);
				if (j >= 0x80)
					hexLeft = 2;
				else
					hexLeft = j + 1;
				nextHexIsArpeggioNoteLength = true;
				append(i);
				append(j);
				return;
			}
			else if (i == 0xE5 && ::songTargetProgram == 1)
			{
				// Don't append yet.  We need to look at the next uint8_t to determine whether to use 0xF3 or 0xE5.
				hexLeft = 3;
				return;
			}
			else if (i == 0xFC && targetAMKVersion == 1)
			{
				//if (tempoRatio != 1) throw AddmusicException("#halvetempo cannot be used on AMK 1 songs that use the $FA $05 or old $FC command.", false, this);
				int channelToCheck;
				if (channel == 8)
					channelToCheck = prevChannel;
				else
					channelToCheck = channel;

				usingFC[channelToCheck] = true;
				currentHex = 0xFC;
				hexLeft = 2;
				// We won't know the gain and delays until later, so don't generate anything else for now.
				return;
			}
			else
			{
				hexLeft = hexLengths[currentHex - 0xDA] - 1;
				if (currentHex == 0xE3)
					guessLength = false;						// NOPE.  Nope nope nope nope nope nope nope nope nope nope.
			}
			if (targetAMKVersion > 1 && targetAMKVersion < 4 && currentHex == 0xFC && remoteGainWarning)
			{
				throw AddmusicException("WARNING: Utilization of the $FC hex command results in an error in AddmusicK 1.0.8 or lower because it was initially replaced with remote code for #amk 2.\nIf you are using this for remote gain, please make sure you're using it like this as it takes five bytes instead of two because of the ability to customize the event type:\n$FC $xx $01 $yy $zz", false, this);
				remoteGainWarning = false;
			}
		}
		else
		{
			hexLeft -= 1;
			
			if (hexLeft == 0 && currentHex == 0xF4 && i >= 0x07 && songTargetProgram == 2 && nonNativeHexWarning) {
				throw AddmusicException("WARNING: A hex command was used which is not native to AddMusicM.\nDid you mean: #amk 1", false, this);
				nonNativeHexWarning = false;
			}
			
			if (hexLeft == 1 && currentHex == 0xFA && songTargetProgram == 2)
			{
				hexLeft = 0;
				throw AddmusicException("This histortical AddmusicM hex command has not yet been implemented into AddmusicK.", false, this);
			}
			
			if (hexLeft == 1 && currentHex == 0xFA)
			{
				currentHexSub = i;
			}
			
			if (hexLeft == 0 && currentHex == 0xFA && currentHexSub == 0x7F)
			{
				//Hot patches require that the $FA $04 VCMD be generated afterwards, not prior.
				//This is because of a bit that handles a special case when the buffer size is set to zero.
				markEchoBufferAllocVCMD();
			}
			
			if (hexLeft == 0 && currentHex == 0xFA && currentHexSub == 0xFE)
			{
				if (i >= 0x80) {
					//Hot patch by bit VCMD has extra bytes if the highest bit is set.
					hexLeft++;
				}
				else {
					markEchoBufferAllocVCMD();
				}	
			}
			
			// If we're on the last hex value for $E5 and this isn't an AMK song, then do some special stuff regarding tremolo.
			// AMK doesn't use $E5 for the tremolo command or sample loading, so it has to emulate them.
			if (hexLeft == 2 && currentHex == 0xE5 && songTargetProgram == 1/*validateTremolo*/)
			{
				//validateTremolo = false;
				if (i >= 0x80)
				{
					hexLeft--;
					if (mySamples.size() == 0 && (i & 0x7F) > 0x13)
						throw AddmusicException("This song uses custom samples, but has not yet defined its samples with the #samples command.", false, this);

					if (optimizeSampleUsage)
						usedSamples[i - 0x80] = true;
					append(0xF3);
					append(i - 0x80);
					return;
				}
				else
				{
					append(0xE5);
				}
			}

			if (hexLeft == 1 && targetAMKVersion > 1 && currentHex == 0xFA && i == 0x05)
				throw AddmusicException("$FA $05 in #amk 2 or above has been replaced with remote code.", false, this);

			// Print error for AM4 songs that attempt to use an invalid FIR filter.  They both A) won't sound like their originals and B) may crash the DSP (or for whatever reason that causes SPCPlayer to go silent with them).
			if (hexLeft == 0 && currentHex == 0xF1 && songTargetProgram == 1)
			{
				if (i > 1)
				{
					char buffer[255];
					sprintf(buffer, "$%02X", i);
					throw AddmusicException(buffer + (std::string)" is not a valid FIR filter for the $F1 command. Must be either $00 or $01.", false, this);
				}
			}

			if (hexLeft == 0 && currentHex == 0xE4 && songTargetProgram == 1)	// AM4 seems to do something strange with $E4?
			{
				i++;
				i &= 0xFF;
			}

			// Do conversion for the old remote gain command.
			if (hexLeft == 1 && currentHex == 0xFC && targetAMKVersion == 1)
			{
				//if (tempoRatio != 1) throw AddmusicException("#halvetempo cannot be used on AMK 1 songs that use the $FA $05 or old $FC command.", false, this);

				int channelToCheck;
				if (channel == 8)
					channelToCheck = prevChannel;
				else
					channelToCheck = channel;


				if (i == 0)
				{
					usingFC[channelToCheck] = false;
					lastFCDelayValue[channelToCheck] = i;
				}
				else
				{
					i = divideByTempoRatio(i, false);
					lastFCDelayValue[channelToCheck] = i;
				}

				return;
			}
			else if (hexLeft == 0 && currentHex == 0xFC && targetAMKVersion == 1)
			{
				//if (tempoRatio != 1) throw AddmusicException("#halvetempo cannot be used on AMK 1 songs that use the $FA $05 or old $FC command.", false, this);
				int channelToCheck;
				if (channel == 8)
					channelToCheck = prevChannel;
				else
					channelToCheck = channel;
				lastFCGainValue[channelToCheck] = i;
				if (i != 0 && lastFCDelayValue[channelToCheck] != 0) {
					//Create a type 5 remote code type event.
					//This remote code event is specifically reserved to replicate the remote gain as a type 2-like remote code event, but it also comes with built-in instrument restoration.
					append(0xFC);
					append(i);
					append(0x01);
					append(0x05);
					append(lastFCDelayValue[channelToCheck]);
				}
				else {
					//There are two ways remote gain won't fire and will instead be canceled: if the timer is zero (meaning it is impossible to trigger in the first place) or if the gain value itself is zero.
					//We will create a type 7 remote code event to stop the type 5 remote code event (and all other non-"key on" events).
					append(0xFC);
					append(0x00);
					append(0x00);
					append(0x07);
					append(0x00);
				}
				return;
			}

			// More code conversion.
			if (hexLeft == 0 && currentHex == 0xFA && targetAMKVersion == 1 && currentHexSub == 0x05)
			{
				//if (tempoRatio != 1) throw AddmusicException("#halvetempo cannot be used on AMK 1 songs that use the $FA $05 or old $FC command.", false, this);
				data[channel].pop_back();					// Remove the last two bytes
				data[channel].pop_back();					// (i.e. the $FA $05)

				int channelToCheck;
				if (channel == 8)
					channelToCheck = prevChannel;
				else
					channelToCheck = channel;

				if (i != 0)
				{
					// We will be using a type 6 remote code event: this is a special reserved type 3-like remote code event that works in conjunction with type 5 (normally only "key on" events get this honor), and also has built-in instrument restoration.
					append(0xFC);
					append(i);
					append(0x01);
					append(0x06);
					append(0x00);
					usingFA[channelToCheck] = true;

				}
				else
				{
					//Create a type 8 remote code event.
					//This remote code event will stop type 6 remote code events. It also stops "key on" remote code events, and only "key on" remote code events.
					append(0xFC);
					append(0x00);
					append(0x00);
					append(0x08);
					append(0x00);

					usingFA[channelToCheck] = false;

				}
				return;
			}

			if (hexLeft == 1 && currentHex == 0xF3)
			{
				if (optimizeSampleUsage)
					usedSamples[i] = true;
			}

			if (hexLeft == 2 && currentHex == 0xF1)
			{
				echoBufferSize = std::max(echoBufferSize, i);
				hasEchoBufferCommand = true;
			}

			if (currentHex == 0xDA && songTargetProgram == 1)			// If this was the instrument command
			{									// And it was >= 0x13
				if (i >= 0x13)							// Then change it to a custom instrument.
					i = i - 0x13 + 30;

				if (optimizeSampleUsage)
					usedSamples[instrumentData[(i - 30) * 5]] = true;
			}

			if (hexLeft == 0 && currentHex == 0xE6)
			{
				if (i == 0)
				{
					if (inE6Loop)
						throw AddmusicException("Cannot nest $E6 loops within other $E6 loops.", false, this);



					inE6Loop = true;

					handleSuperLoopEnter();
				}
				else
				{
					if (!inE6Loop)
						throw AddmusicException("An E6 loop starting point has not yet been declared.", false, this);
					inE6Loop = false;

					handleSuperLoopExit(i + 1);
				}
			}

			if (hexLeft == 0 && currentHex == 0xF4)
			if (i == 0x00 || i == 0x06)
				hasYoshiDrums = true;

			if (hexLeft == 1 && currentHex == 0xDD)			// Hack allowing the $DD command to accept a note as a parameter.
			{
				int backUpPos = pos;
				while (true)
				{
					skipSpaces();
					if (text[pos] == 'o')
					{
						if (targetAMKVersion < 4 && octaveForDDWarning) {
							throw AddmusicException("WARNING: Using o after reading $DD will freeze on hex validation for AddmusicK 1.0.8 and lower!", false, this);
							octaveForDDWarning = false;
						}
						pos++;
						getInt();
					}
					else if (text[pos] == 'a' || text[pos] == 'b' || text[pos] == 'c' || text[pos] == 'd' || text[pos] == 'e' || text[pos] == 'f' || text[pos] == 'g' ||
						 text[pos] == 'A' || text[pos] == 'B' || text[pos] == 'C' || text[pos] == 'D' || text[pos] == 'E' || text[pos] == 'F' || text[pos] == 'G')
					{
						if (updateQ[channel] == true)
							throw AddmusicException("You cannot use a note as the last parameter of the $DD command if you've also\nused the qXX command just before it.", false, this);
							hexLeft = 0;
						nextNoteIsForDD = true;
						break;
					}
					else if (text[pos] == '<' || text[pos] == '>')
					{
						pos++;
					}
					else
						break;
				}
				pos = backUpPos;
			}

			// Pan fade tempo adjustment
			if (currentHex == 0xDC && hexLeft == 1) i = divideByTempoRatio(i, false);

			// Pitch bend tempo adjustment
			if (currentHex == 0xDD && hexLeft == 2) i = divideByTempoRatio(i, false);
			if (currentHex == 0xDD && hexLeft == 1) i = divideByTempoRatio(i, false);

			// Vibrato tempo adjustment
			if (currentHex == 0xDE && hexLeft == 2) i = divideByTempoRatio(i, false);
			if (currentHex == 0xDE && hexLeft == 1) i = multiplyByTempoRatio(i);

			// Global volume fade tempo adjustment
			if (currentHex == 0xE1 && hexLeft == 1) i = divideByTempoRatio(i, false);

			// Tempo tempo adjustment (?!)
			if (currentHex == 0xE2 && hexLeft == 0) i = divideByTempoRatio(i, false);

			// Tempo fade tempo adjustment
			if (currentHex == 0xE3 && hexLeft == 1) i = divideByTempoRatio(i, false);

			// Tremolo tempo adjustment
			if (currentHex == 0xE5 && hexLeft == 2) i = divideByTempoRatio(i, false);
			if (currentHex == 0xE5 && hexLeft == 1) i = multiplyByTempoRatio(i);

			// Volume fade tempo adjustment
			if (currentHex == 0xE8 && hexLeft == 1) i = divideByTempoRatio(i, false);

			// Vibrato fade tempo adjustment
			if (currentHex == 0xEA && hexLeft == 0) i = divideByTempoRatio(i, false);

			// Pitch envelope release tempo adjustment
			if (currentHex == 0xEB && hexLeft == 2) i = divideByTempoRatio(i, false);
			if (currentHex == 0xEB && hexLeft == 1) i = divideByTempoRatio(i, false);

			// Pitch envelope attack tempo adjustment
			if (currentHex == 0xEC && hexLeft == 2) i = divideByTempoRatio(i, false);
			if (currentHex == 0xEC && hexLeft == 1) i = divideByTempoRatio(i, false);

			// Echo fade tempo adjustment
			if (currentHex == 0xF2 && hexLeft == 2) i = divideByTempoRatio(i, false);

			// Arpeggio (etc.) tempo adjustment
			if (nextHexIsArpeggioNoteLength == true) { i = divideByTempoRatio(i, false); nextHexIsArpeggioNoteLength = false; }

		}



		if (i == -1)
		{
			throw AddmusicException("Error parsing hex command.", false, this);
			return;
		}

		if (i < 0 || i > 255)
		{
			throw AddmusicException("Illegal value for hex command.", false, this);
			return;
		}
	}
	append(i);
}

void Music::markEchoBufferAllocVCMD()
{
	if (!echoBufferAllocVCMDIsSet && resizedChannel != -1 && channel != 8 && channel == resizedChannel && !passedNote[channel] && !hasEchoBufferCommand && !passedIntro[channel])
		{
			//Mark the location to generate the echo buffer allocation VCMD.
			//This won't be performed if this is located past a note or a loop marker, since being
			//past a note means a note will key on prior to the song initializing properly and being
			//past a loop marker will mean the VCMD will retrigger, which may or may not end well.
			echoBufferAllocVCMDIsSet = true;
			echoBufferAllocVCMDLoc = data[channel].size()+1;
			echoBufferAllocVCMDChannel = channel;
		}
}

void Music::parseNote()
{
	if (isupper(text[pos]) && targetAMKVersion < 4 && caseNoteWarning){
		throw AddmusicException("WARNING: Upper case letters will not translate correctly on AddmusicK 1.0.8 or lower! Your build may have different results!", false, this);
		caseNoteWarning = false;
	}
	if (channel != 8) {
		passedNote[channel] = true;
	}
	else {
		passedNote[prevChannel] = true;
	}
	j = tolower(text[pos]);
	pos++;

	if (inRemoteDefinition)
		throw AddmusicException("Remote definitions cannot contain note data!", false, this);

	if (songTargetProgram == 0 && channelDefined == false && inRemoteDefinition == false)
		throw AddmusicException("Note data must be inside a channel!", false, this);

	if (j == 'r')
		i = 0xC7;
	else if (j == '^')
		i = 0xC6;
	else
	{
		//am4silence++;
		i = getPitch(j);

		if (usingHTranspose)
			i += hTranspose;
		else
		{
			if (!ignoreTuning[channel])				// More AM4 tuning stuff
				i -= transposeMap[instrument[channel]];
		}

		if (i < 0x80)
		{
			if (songTargetProgram == 0 && targetAMKVersion < 4 && lowNoteWarning) {
				throw AddmusicException("WARNING: This older AddmusicK song outputs an invalid note byte (its pitch is too low)! It may not be audible in the song!", false, this);
				lowNoteWarning = false; 
			}
			else {
				throw AddmusicException("Note's pitch was too low.", false, this);
				i = 0xC7;
			}
		}
		else if (i >= 0xC6)
		{
			throw AddmusicException("Note's pitch was too high.", false, this);
		}
		else if (instrument[channel] >= 21 && instrument[channel] < 30 && i < 0xC6)
		{
			i = 0xD0 + (instrument[channel] - 21);

			if (songTargetProgram != 0 || ((channel == 6 || channel == 7 || (channel == 8 && (prevChannel == 6 || prevChannel == 7))) == false))	// If this is not a SFX channel,
				instrument[channel] = 0xFF;										// Then don't force the drum pitch on every note.
		}
	}





	if (inPitchSlide)
	{
		inPitchSlide = false;
		append(0xDD);
		append(0x00);
		append(prevNoteLength);
		append(i);

	}

	if (nextNoteIsForDD)
	{
		append(i);
		nextNoteIsForDD = false;
		return;
	}
	nextNoteIsForDD = false;

	j = 0;

	bool okayToRewind = false;

	do
	{
		int tempsize = j;	// If there's a pitch bend up ahead, we need to not optimize the last tie.
		int temppos = pos;	//

		if (j != 0 && (text[pos] == '^' || (i == 0xC7 && text[pos] == 'r')))
			pos++;

		j += getNoteLength(getInt());
		skipSpaces();

		if ((strncmp(text.c_str() + pos, "$DD", 3) == 0 || strncmp(text.c_str() + pos, "$dd", 3) == 0 || (songTargetProgram != 0 && strncmp(text.c_str() + pos, "&", 1) == 0)) && okayToRewind)
		{
			j = tempsize;		//
			pos = temppos;		// "Rewind" so we forcibly place a tie before the bend.
			break;			//
		}
		okayToRewind = true;

		if (pos >= text.length())
			break;

	} while (text[pos] == '^' || (i == 0xC7 && text[pos] == 'r'));

	/*if (normalLoopInsideE6Loop)
	tempLoopLength += j;
	else if (normalLoopInsideE6Loop)
	e6LoopLength += j;
	else if (::inE6Loop)
	e6LoopLength += j;
	else if (channel == 8)
	tempLoopLength += j;
	else
	lengths[channel] += j;*/

	j = divideByTempoRatio(j, true);

	addNoteLength(j);

	if (j >= divideByTempoRatio(0x80, true))		// Note length must be less than 0x80
	{
		append(divideByTempoRatio(0x60, true));

		if (updateQ[channel])
		{
			append(q[channel]);
			updateQ[channel] = false;
			updateQ[8] = false;
			noteParamaterByteCount++;
		}
		append(i); j -= divideByTempoRatio(0x60, true);

		while (j > divideByTempoRatio(0x60, true))
		{
			append(0xC6); j -= divideByTempoRatio(0x60, true);
		}

		if (j > 0)
		{
			if (j != divideByTempoRatio(0x60, true)) append(j);
			append(0xC6);
		}
		prevNoteLength = j;
		return;
	}
	else if (j > 0)
	{
		if (j != prevNoteLength || updateQ[channel])
			append(j);
		prevNoteLength = j;
		if (updateQ[channel])
		{
			append(q[channel]);
			updateQ[channel] = false;
			updateQ[8] = false;
			noteParamaterByteCount++;
		}
		append(i);
	}
	//append(i);
}
void Music::parseHDirective()
{
	if (songTargetProgram == 1 && nonNativeCmdWarning) {
		throw AddmusicException("WARNING: A command was used which is not native to AddMusic405.\nDid you mean: #amm", false, this);
		nonNativeCmdWarning = false;
	}
	pos++;
	//bool negative = false;

	//if (text[pos] == '-')
	//{
	//	negative = true;
	//	pos++;
	//}
	try
	{
		i = getIntWithNegative();
	}
	catch (...)
	{
		throw AddmusicException("Error parsing h transpose directive.", false, this);
	}
	//if (negative) i = -i;
	//transposeMap[instrument[channel]] = -i;
	//htranspose[instrument[channel]] = true;
	hTranspose = i;
	usingHTranspose = true;
}
void Music::parseNCommand()
{
	pos++;
	i = getHex();
	if (i < 0 || i > 0x1F)
		throw AddmusicException("Invlid value for the n command.  Value must be in hex and between 0 and 1F.", false, this);

		append(0xF8);
	append(i);
}

void Music::parseOptionDirective()
{
	if (targetAMKVersion == 1)
		throw AddmusicException("Unknown command.", false, this);

	if (channelDefined == true)
		throw AddmusicException("#option directives must be used before any and all channels.", false, this);

	skipSpaces();

	if (strnicmp(text.c_str() + pos, "smwvtable", 9) == 0 && isspace(text[pos + 9]))
	{
		pos += 9;
		if (usingSMWVTable == false)
		{
			append(0xFA);
			append(0x06);
			append(0x00);
			usingSMWVTable = true;
		}
		else
		{
			throw AddmusicException("This song is already using the SMW V Table. This command is just wasting three bytes...", false, this);
		}
	}
	else if (strnicmp(text.c_str() + pos, "nspcvtable", 10) == 0 && isspace(text[pos + 10]))
	{
		pos += 10;
		append(0xFA);
		append(0x06);
		append(0x01);
		usingSMWVTable = false;

		throw AddmusicException("This song uses the N-SPC V by default. This command is just wasting two bytes...", false, this);
	}
	else if (strnicmp(text.c_str() + pos, "tempoimmunity", 13) == 0 && isspace(text[pos + 13]))
	{
		pos += 13;
		append(0xF4);
		append(0x07);
	}
	else if (strnicmp(text.c_str() + pos, "noloop", 6) == 0 && isspace(text[pos + 6]))
	{
		pos += 6;
		doesntLoop = true;
	}
	else if (strnicmp(text.c_str() + pos, "dividetempo", 11) == 0 && isspace(text[pos + 11]))
	{
		pos += 11;
		skipSpaces();
		i = getInt();
		if (i == -1)
			throw AddmusicException("Missing integer argument for #option dividetempo.", false, this);
		if (i == 0)
			throw AddmusicException("Argument for #option dividetempo cannot be 0.", false, this);
		if (i == 1)
			throw AddmusicException("#option dividetempo 1 has no effect besides printing this warning.", false, this);

		tempoRatio = i;

		if (tempoRatio < 0)
			throw AddmusicException("#halvetempo has been used too many times...what are you even doing?", false, this);
	}
	else if (targetAMKVersion >= 4 && strnicmp(text.c_str() + pos, "amk109hotpatch", 14) == 0 && isspace(text[pos + 14]))
	{
		pos += 14;
		append(0xFA);
		append(0x7F);
		append(0x01);
		markEchoBufferAllocVCMD();
		//Prevent an off by one error (normally this is offset by one due to the last hex parameter byte being added after it), but for the #option itself, we shouldn't do this).
		echoBufferAllocVCMDLoc--;
	}
	else
	{
		throw AddmusicException("#option directive missing its first argument", false, this);
	}
}


void Music::parseSpecialDirective()
{
	if (strnicmp(text.c_str() + pos, "instruments", 11) == 0 && isspace(text[pos + 11]))
	{
		pos += 11;
		parseInstrumentDefinitions();

	}
	else if (strnicmp(text.c_str() + pos, "samples", 7) == 0 && isspace(text[pos + 7]))
	{
		pos += 7;
		parseSampleDefinitions();
		pos++;
	}
	else if (strnicmp(text.c_str() + pos, "pad", 3) == 0 && isspace(text[pos + 3]))
	{
		pos += 3;
		parsePadDefinition();
	}
	else if (strnicmp(text.c_str() + pos, "define", 6) == 0 && isspace(text[pos + 6]))
	{
		pos += 6;
		parseDefine();
	}
	else if (strnicmp(text.c_str() + pos, "undef", 5) == 0 && isspace(text[pos + 5]))
	{
		pos += 5;
		parseUndef();
	}
	else if (strnicmp(text.c_str() + pos, "ifdef", 5) == 0 && isspace(text[pos + 5]))
	{
		pos += 5;
		parseIfdef();
	}
	else if (strnicmp(text.c_str() + pos, "ifndef", 6) == 0 && isspace(text[pos + 6]))
	{
		pos += 6;
		parseIfndef();
	}
	else if (strnicmp(text.c_str() + pos, "endif", 5) == 0 && isspace(text[pos + 5]))
	{
		pos += 5;
		parseEndif();
	}
	else if (strnicmp(text.c_str() + pos, "spc", 3) == 0 && isspace(text[pos + 3]))
	{
		pos += 3;
		parseSPCInfo();
	}
	else if (strnicmp(text.c_str() + pos, "louder", 6) == 0 && isspace(text[pos + 6]))
	{
		if (targetAMKVersion > 1)
			throw AddmusicException("#louder is redundant in #amk 2 and above.", false, this);
		pos += 6;
		parseLouderCommand();
	}
	else if (strnicmp(text.c_str() + pos, "tempoimmunity", 13) == 0 && isspace(text[pos + 13]))
	{
		pos += 13;
		append(0xF4);
		append(0x07);
	}
	else if (strnicmp(text.c_str() + pos, "path", 4) == 0 && isspace(text[pos + 4]))
	{
		pos += 4;
		parsePath();
	}
	else if (strnicmp(text.c_str() + pos, "am4", 3) == 0 && isspace(text[pos + 3]))
	{
		pos += 3;
	}
	else if (strnicmp(text.c_str() + pos, "amm", 3) == 0 && isspace(text[pos + 3]))
	{
		pos += 3;
	}
	else if (strnicmp(text.c_str() + pos, "amk=", 4) == 0)
	{
		pos += 4;
		getInt();
	}
	else if (strnicmp(text.c_str() + pos, "halvetempo", 10) == 0)
	{
		pos += 10;
		if (channelDefined == true)
			throw AddmusicException("#halvetempo must be used before any and all channels.", false, this);
		tempoRatio *= 2;


		if (tempoRatio < 0)
			throw AddmusicException("#halvetempo has been used too many times...what are you even doing?", false, this);
	}
	else if (strnicmp(text.c_str() + pos, "option", 6) == 0 && isspace(text[pos + 6]))
	{
		pos += 6;
		parseOptionDirective();
	}
}

void Music::parseReplacementDirective()
{
	sortReplacements = true;
	pos++;

	int quotedStringLength = 0;

	std::string s = getQuotedString(text, pos, quotedStringLength);
	std::string find, replacement;

	i = s.find('=');

	if (i == -1)
		throw AddmusicException("Error parsing replacement directive; could not find '='", true, this);

	find = s.substr(0, i);
	replacement = s.substr(i + 1);

	pos += quotedStringLength + 1;

	while (isspace(find[find.length() - 1]))
	{
		find.erase(find.end() - 1, find.end());
		if (find.length() == 0)
			throw AddmusicException("Error parsing replacement directive; string to find was of zero length.", true, this);
	}

	if (replacement.length() != 0)
	{
		while (isspace(replacement[0]))
		{
			replacement.erase(replacement.begin(), replacement.begin() + 1);
		}
	}

	replacements[find] = replacement;

	//std::sort(replacements.begin(), replacements.end(), sortFunction);
}

void Music::parseInstrumentDefinitions()
{
	enum parseState {
		lookingForOpenBrace,
		lookingForAnything,
		lookingForDollarSign,
		lookingForOpenQuote,
		gettingName,
		gettingValue,
	};

	parseState state = lookingForOpenBrace;

	skipSpaces();

	if (text[pos++] != '{')
		throw AddmusicException("Could not find opening curly brace in instrument definition.", true, this);

	skipSpaces();
	while (text[pos] != '}')
	{
		if (text[pos] != '"' && text[pos] != '@'&& text[pos] != 'n')
			throw AddmusicException("Error parsing instrument definition.", false, this);
		bool isName, isNoise;
		if (text[pos] == '"') isName = true, isNoise = false;
		else if (text[pos] == '@') isName = false, isNoise = false;
		else if (text[pos] == 'n') isName = false, isNoise = true;
		else
		{
			pos++;
			throw AddmusicException("Error parsing instrument definition.", true, this);
		}
		pos++;
		if (isName)
		{
			std::string brrName;
			while (text[pos] != '"')
			{
				if (pos >= text.size()) throw AddmusicException("Error parsing sample portion of the instrument definition.", true, this);
				brrName += text[pos++];
			}
			pos++;
			i = -1;
			brrName = basepath + brrName;
			int gs = getSample(brrName, this);
			for (j = 0; j < mySamples.size(); j++)
			{
				if (mySamples[j] == gs)
				{
					i = j;
					break;
				}
			}


			if (i == -1)
				throw AddmusicException("The specified sample was not included in this song.", true, this);
		}
		else if (isNoise)
		{
			i = getHex();
			if (i == -1 || i > 0xFF)
				throw AddmusicException("Error parsing the noise portion of the instrument command.", true, this);

			if (i >= 0x20)
				throw AddmusicException("Invalid noise pitch.  The value must be a hex value from 0 - 1F.", true, this);

				i |= 0x80;
		}
		else
		{
			i = getInt();
			if (i == -1)
				throw AddmusicException("Error parsing the instrument copy portion of the instrument command.", true, this);

			if (i >= 30)
				throw AddmusicException("Cannot use a custom instrument's sample as a base for another custom instrument.", true, this);

				i = instrToSample[i];
		}

		instrumentData.push_back(i);
		if (optimizeSampleUsage)
			usedSamples[i] = true;

		skipSpaces();

		for (j = 0; j < 5; j++)
		{
			skipSpaces();
			if (text[pos++] != '$') throw AddmusicException("Error parsing instrument definition; there were too few bytes following the sample (there must be 6).", true, this);
			i = getHex();
			if (i == -1 || i > 0xFF) throw AddmusicException("Error parsing instrument definition.", true, this);
			instrumentData.push_back(i);
		}
		skipSpaces();

	}
	pos++;
	return;

	/*	//unsigned char temp;
	int count = 0;

	while (pos < text.length())
	{
	switch (state)
	{
	case lookingForOpenBrace:
	if (isspace(text[pos])) break;
	if (text[pos] != '{')
	throw AddmusicException("Could not find opening curly brace in instrument definition.", false, this);
	state = lookingForDollarSign;
	break;
	case lookingForDollarSign:
	if (text[pos] == '\n')
	count = 0;
	if (isspace(text[pos])) break;
	if (text[pos] == '$')
	{
	if (count == 6) throw AddmusicException("Invalid number of arguments for instrument.  Total number of bytes must be a multiple of 6.", false, this);
	state = gettingValue;
	break;
	}
	if (text[pos] == '}')
	{
	if (count != 0)
	throw AddmusicException("Invalid number of arguments for instrument.  Total number of bytes must be a multiple of 6.", false, this);
	pos++;
	return;
	}

	throw AddmusicException("Error parsing instrument definition.", false, this);
	break;
	case gettingValue:
	int val = getHex();
	if (val == -1 || val > 255) throw AddmusicException("Error parsing instrument definition.", false, this);
	instrumentData.push_back(val);
	state = lookingForDollarSign;
	count++;
	break;
	}
	pos++;
	}*/
}

void Music::parseSampleDefinitions()
{
	skipSpaces();

	if (text[pos++] != '{')
		throw AddmusicException("Unexpected character in sample group definition.  Expected \"{\".", true, this);


	while (text[pos] != '}')
	{
		if (pos >= text.size())
		EOFTooEarly:												// Oh, laziness.
		throw AddmusicException("Unexpected end of file found while parsing sample group definition.", true, this);

			skipSpaces();

		if (text[pos] == '\"')
		{
			pos++;
			int quotedStringLength = 0;
			std::string tempstr = basepath + getQuotedString(text, pos, quotedStringLength);
			std::string extension;
			auto tmppos = tempstr.find_last_of(".");
			if (tmppos == -1)
				throw AddmusicException("The filename for the sample was missing its extension; is it a .brr or .bnk?", true, this);
				extension = tempstr.substr(tmppos);
			if (extension == ".bnk")
				addSampleBank(tempstr, this);
			else if (extension == ".brr")
				addSample(tempstr, this, true);
			else
				throw AddmusicException("The filename for the sample was invalid.  Only \".brr\" and \".bnk\" are allowed.", true, this);

				pos += quotedStringLength + 1;
		}
		else if (text[pos] == '#')
		{
			pos++;
			std::string tempstr;
			while (isspace(text[pos]) == false)
			{
				if (pos >= text.size()) goto EOFTooEarly;
				tempstr += text[pos];
				pos++;
			}

			addSampleGroup(tempstr, this);
		}
		else if (text[pos] == '}')
			break;
		else if (isspace(text[pos]) == false)
		{
			throw AddmusicException("Unexpected character found in sample group definition.", true, this);
		}

	}

}

void Music::parsePadDefinition()
{
	skipSpaces();
	if (text[pos] != '$')
		throw AddmusicException("Error parsing padding directive.", false, this);
		pos++;
	i = getHex(true);
	if (i == -1)
		throw AddmusicException("Error parsing padding directive.", false, this);

		minSize = i;

}

void Music::parseLouderCommand()
{
	append(0xF4);
	append(0x08);
}

void Music::parsePath()
{
	skipSpaces();

	if (text[pos] != '\"')
		throw AddmusicException("Unexpected symbol found in path command.  Expected a quoted string.", false, this);

	pos++;
	int quotedStringLength = 0;

	basepath = "./" + getQuotedString(text, pos, quotedStringLength) + "/";

	pos += quotedStringLength + 1;
}

int Music::getInt()
{
	//if (text[pos] == '$') { pos++; return getHex(); }	// Allow for things such as t$20 instead of t32.
	// Actually, can't do it.
	// Consider this:
	// l8r$ED$00$00
	// Yeah. Oh well.
	// Attempt to do a replacement.  (So things like "ab = 8"    [c1]ab    are valid).
	doReplacement();

	int i = 0;
	int d = 0;

	while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9')
	{
		d++;
		i = (i * 10) + text[pos] - '0';
		pos++;
	}

	if (d == 0) return -1; else return i;
}

int Music::getIntWithNegative()
{

	doReplacement();

	int i = 0;
	int d = 0;
	bool n = false;
	if (text[pos] == '-')
	{
		n = true;
		pos++;
	}

	while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9')
	{
		d++;
		i = (i * 10) + text[pos] - '0';
		pos++;
	}

	if (d == 0)
		throw "Invalid number";
	else
	{
		if (n) return -i;
		else return i;
	}
}

int Music::getInt(const std::string &str, int &p)
{

	doReplacement();

	int i = 0;
	int d = 0;

	while (p < str.size() && str[p] >= '0' && str[p] <= '9')
	{
		d++;
		i = (i * 10) + str[p] - '0';
		p++;
	}

	if (d == 0) return -1; else return i;
}

int Music::getHex(bool anyLength)
{
	doReplacement();


	int i = 0;
	int d = 0;
	int j;

	while (pos < text.size())
	{
		if (d >= 2 && anyLength == false)
			break;

		if ('0' <= text[pos] && text[pos] <= '9') j = text[pos] - 0x30;
		else if ('A' <= text[pos] && text[pos] <= 'F') j = text[pos] - 0x37;
		else if ('a' <= text[pos] && text[pos] <= 'f') j = text[pos] - 0x57;
		else break;
		pos++;
		d++;
		i = (i * 16) + j;
	}
	if (d == 0) return -1;

	return i;
}

int Music::getPitch(int i)
{
	static const int pitches[] = { 9, 11, 0, 2, 4, 5, 7 };

	i = pitches[i - 0x61] + (octave - 1) * 12 + 0x80;

	if (text[pos] == '+') { i++; pos++; }
	else if (text[pos] == '-') { i--; pos++; }

	/*if (i < 0x80)
	return -1;
	if (i >= 0xC6)
	return -2;*/

	return i;
}

int Music::getNoteLength(int i)
{
	//bool still = true;

	if (i == -1 && text[pos] == '=')
	{
		pos++;
		i = getInt();
		if (i == -1) //throw AddmusicException("Error parsing note.", false, this);
		{
			throw AddmusicException("Error parsing note", false, this);
		}
		if (targetAMKVersion < 4)
		{
			return i;
		}
		//if (i < 1) still = false; else return i;
	}

	//if (still)
	//{
	else if (i < 1 || i > 192) i = defaultNoteLength;
	else {
		if (192 % i != 0 && fractionNoteLengthWarning) {
			throw AddmusicException("WARNING: A note length was used that is not divisible by 192 ticks, and thus results in a fractional tick value.", false, this);
			fractionNoteLengthWarning = false;
		}
		i = 192 / i;
	}

	return getNoteLengthModifier(i, true);
}

int Music::getNoteLengthModifier(int i , bool allowTriplet) {
	int frac = i;

	int times = 0;
	while (pos < text.size() && text[pos] == '.')
	{
		if (frac % 2 != 0 && fractionNoteLengthWarning) {
			if (times != 0) {
				char buffer[255];
				sprintf(buffer, "%i", times+1);
				throw AddmusicException((std::string)"WARNING: Adding " + buffer + " dots to this note results in a fractional tick value.", false, this);
			}
			else {
				throw AddmusicException("WARNING: Adding a dot to this note results in a fractional tick value.", false, this);
			}
			fractionNoteLengthWarning = false;
		}
		frac = frac / 2;
		i += frac;
		pos++;
		times++;
		if (times == 2 && songTargetProgram == 1) break;	// AM4 only allows two dots for whatever reason.
	}
	//}
	if (triplet && allowTriplet) {
		if (i % 3 != 0 && fractionNoteLengthWarning) {
			throw AddmusicException("WARNING: Putting this note in a triplet results in a fractional tick value.", false, this);
			fractionNoteLengthWarning = false;
		}
		i = (int)floor(((double)i * 2.0 / 3.0) + 0.5);
	}
	return i;
}


bool sortTempoPair(const std::pair<double, int> &p1, const std::pair<double, int> &p2)
{
	if (p1.first == p2.first)
		return p1.second < p2.second;

	return p1.first < p2.first;
}

void Music::pointersFirstPass()
{
	if (errorCount)
		throw AddmusicException("There were errors when compiling the music file.  Compilation aborted. Your ROM has not been modified.", true, this);

	if (data[0].size() == 0 && data[1].size() == 0 && data[2].size() == 0 && data[3].size() == 0 && data[4].size() == 0 && data[5].size() == 0 && data[6].size() == 0 && data[7].size() == 0)
		throw AddmusicException("This song contained no musical data!", false, this);

	if (resizedChannel != -1)
	{
		int z = 0;
		if (targetAMKVersion > 1)
		{
			data[resizedChannel].insert(data[resizedChannel].begin(), 0x01);
			data[resizedChannel].insert(data[resizedChannel].begin(), 0x06);
			data[resizedChannel].insert(data[resizedChannel].begin(), 0xFA);
			z += 3;
		}
		if (targetAMKVersion == 1)
		{
			data[resizedChannel].insert(data[resizedChannel].begin(), 0x02);
			data[resizedChannel].insert(data[resizedChannel].begin(), 0x7F);
			data[resizedChannel].insert(data[resizedChannel].begin(), 0xFA);
			z += 3;
		}
		else if (songTargetProgram == 1)
		{
			data[resizedChannel].insert(data[resizedChannel].begin(), 0x04);
			data[resizedChannel].insert(data[resizedChannel].begin(), 0x7F);
			data[resizedChannel].insert(data[resizedChannel].begin(), 0xFA);
			z += 3;
		}
		else if (songTargetProgram == 2)
		{
			data[resizedChannel].insert(data[resizedChannel].begin(), 0x05);
			data[resizedChannel].insert(data[resizedChannel].begin(), 0x7F);
			data[resizedChannel].insert(data[resizedChannel].begin(), 0xFA);
			z += 3;
		}
		if (echoBufferSize > 0 || !echoBufferAllocVCMDIsSet || hasEchoBufferCommand) {
			//Just put the VCMD in its default place: no need to move it around.
			//In particular, the $F1 command means that echo writes have been enabled, meaning the special case is irrelevant.
			data[resizedChannel].insert(data[resizedChannel].begin(), echoBufferSize);
			data[resizedChannel].insert(data[resizedChannel].begin(), 0x04);
			data[resizedChannel].insert(data[resizedChannel].begin(), 0xFA);
			z += 3;
		}
		else
		{
			//Generate the echo buffer allocation command after the hot patch VCMD, but before all echo VCMDs and notes.
			data[echoBufferAllocVCMDChannel].insert(data[echoBufferAllocVCMDChannel].begin()+echoBufferAllocVCMDLoc+3, echoBufferSize);
			data[echoBufferAllocVCMDChannel].insert(data[echoBufferAllocVCMDChannel].begin()+echoBufferAllocVCMDLoc+3, 0x04);
			data[echoBufferAllocVCMDChannel].insert(data[echoBufferAllocVCMDChannel].begin()+echoBufferAllocVCMDLoc+3, 0xFA);
			//The channel this command is inserted into gets its pointers recalibrated.
			for (int a = 0; a < loopLocations[echoBufferAllocVCMDChannel].size(); a++) {
				loopLocations[echoBufferAllocVCMDChannel][a] += 3;
			}
			for (int a = 0; a <= 1; a++) {
				phrasePointers[echoBufferAllocVCMDChannel][a] += 3;
			}
		}
		//All pointers that were previously output must be recalibrated for the channel.
		//This specifically involves phrase pointers, loop locations and remote gain positions.
		//Why isn't this done sooner? Because we don't know whether some of these are even going to be in there in the first place.
		for (int a = 0; a < loopLocations[resizedChannel].size(); a++) {
			loopLocations[resizedChannel][a] += z;
		}
		for (int a = 0; a <= 1; a++) {
			phrasePointers[resizedChannel][a] += z;
		}
	}

	for (int z = 0; z < 8; z++)
	{
		if (data[z].size() != 0)
		{
			channel = z;
			append(0);
		}
	}



	if (mySamples.size() == 0)	// If no sample groups were provided...
	{
		text = "{#default }";		// This is a dumb, cheap trick, but...eh.
		pos = 0;
		parseSampleDefinitions();
	}

	if (game.empty())
		game = "Super Mario World (custom)";

	if (optimizeSampleUsage)
	{
		int emptySampleIndex = getSample("EMPTY.brr", this);
		if (emptySampleIndex == -1)
		{
			addSample("EMPTY.brr", this, true);
			emptySampleIndex = getSample("EMPTY.brr", this);
		}


		for (i = 0; i < mySamples.size(); i++)
		if (usedSamples[i] == false && samples[mySamples[i]].important == false)
			mySamples[i] = emptySampleIndex;
	}

	pos = 0;	// Pos no longer means text file position.

	if (data[0].size()) phrasePointers[0][0] = 0;
	pos = data[0].size();

	if (data[1].size()) phrasePointers[1][0] = pos;
	pos += data[1].size();

	if (data[2].size()) phrasePointers[2][0] = pos;
	pos += data[2].size();

	if (data[3].size()) phrasePointers[3][0] = pos;
	pos += data[3].size();

	if (data[4].size()) phrasePointers[4][0] = pos;
	pos += data[4].size();

	if (data[5].size()) phrasePointers[5][0] = pos;
	pos += data[5].size();

	if (data[6].size()) phrasePointers[6][0] = pos;
	pos += data[6].size();

	if (data[7].size()) phrasePointers[7][0] = pos;

	for (i = 0; i < 8; i++)
		phrasePointers[i][1] += phrasePointers[i][0];

	playOnce = doesntLoop;

	spaceForPointersAndInstrs = 20;

	if (hasIntro)
		spaceForPointersAndInstrs += 18;
	if (!doesntLoop)
		spaceForPointersAndInstrs += 2;

	spaceForPointersAndInstrs += instrumentData.size();

	allPointersAndInstrs.resize(spaceForPointersAndInstrs);// = alloc(spaceForPointersAndInstrs);
	//for (i = 0; i < spaceForPointers; i++) allPointers[i] = 0x55;

	int add = (hasIntro ? 2 : 0) + (doesntLoop ? 0 : 2) + 4;

	//memcpy(allPointersAndInstrs.data() + add, instrumentData.base, instrumentData.size());
	for (i = 0; i < instrumentData.size(); i++)
		allPointersAndInstrs[i + add] = instrumentData[i];

	allPointersAndInstrs[0] = (add + instrumentData.size()) & 0xFF;
	allPointersAndInstrs[1] = ((add + instrumentData.size()) >> 8) & 0xFF;

	if (doesntLoop)
	{
		allPointersAndInstrs[add - 2] = 0xFF;	// Will be re-evaluated to 0000 when the pointers are adjusted later.
		allPointersAndInstrs[add - 1] = 0xFF;
	}
	else
	{
		allPointersAndInstrs[add - 4] = 0xFE;	// Will be re-evaluated to FF00 when the pointers are adjusted later.
		allPointersAndInstrs[add - 3] = 0xFF;
		if (hasIntro)
		{
			allPointersAndInstrs[add - 2] = 0xFD;	// Will be re-evaluated to 0002 + ARAMPos when the pointers are adjusted later.
			allPointersAndInstrs[add - 1] = 0xFF;
		}
		else
		{
			allPointersAndInstrs[add - 2] = 0xFC;	// Will be re-evaluated to ARAMPos when the pointers are adjusted later.
			allPointersAndInstrs[add - 1] = 0xFF;
		}
	}
	if (hasIntro)
	{
		allPointersAndInstrs[2] = (add + instrumentData.size() + 16) & 0xFF;
		allPointersAndInstrs[3] = (add + instrumentData.size() + 16) >> 8;
	}

	add += instrumentData.size();
	allPointersAndInstrs[0 + add] = data[0].size() != 0 ? (phrasePointers[0][0] + spaceForPointersAndInstrs) & 0xFF : 0xFB;
	allPointersAndInstrs[1 + add] = data[0].size() != 0 ? (phrasePointers[0][0] + spaceForPointersAndInstrs) >> 8 : 0xFF;
	allPointersAndInstrs[2 + add] = data[1].size() != 0 ? (phrasePointers[1][0] + spaceForPointersAndInstrs) & 0xFF : 0xFB;
	allPointersAndInstrs[3 + add] = data[1].size() != 0 ? (phrasePointers[1][0] + spaceForPointersAndInstrs) >> 8 : 0xFF;
	allPointersAndInstrs[4 + add] = data[2].size() != 0 ? (phrasePointers[2][0] + spaceForPointersAndInstrs) & 0xFF : 0xFB;
	allPointersAndInstrs[5 + add] = data[2].size() != 0 ? (phrasePointers[2][0] + spaceForPointersAndInstrs) >> 8 : 0xFF;
	allPointersAndInstrs[6 + add] = data[3].size() != 0 ? (phrasePointers[3][0] + spaceForPointersAndInstrs) & 0xFF : 0xFB;
	allPointersAndInstrs[7 + add] = data[3].size() != 0 ? (phrasePointers[3][0] + spaceForPointersAndInstrs) >> 8 : 0xFF;
	allPointersAndInstrs[8 + add] = data[4].size() != 0 ? (phrasePointers[4][0] + spaceForPointersAndInstrs) & 0xFF : 0xFB;
	allPointersAndInstrs[9 + add] = data[4].size() != 0 ? (phrasePointers[4][0] + spaceForPointersAndInstrs) >> 8 : 0xFF;
	allPointersAndInstrs[10 + add] = data[5].size() != 0 ? (phrasePointers[5][0] + spaceForPointersAndInstrs) & 0xFF : 0xFB;
	allPointersAndInstrs[11 + add] = data[5].size() != 0 ? (phrasePointers[5][0] + spaceForPointersAndInstrs) >> 8 : 0xFF;
	allPointersAndInstrs[12 + add] = data[6].size() != 0 ? (phrasePointers[6][0] + spaceForPointersAndInstrs) & 0xFF : 0xFB;
	allPointersAndInstrs[13 + add] = data[6].size() != 0 ? (phrasePointers[6][0] + spaceForPointersAndInstrs) >> 8 : 0xFF;
	allPointersAndInstrs[14 + add] = data[7].size() != 0 ? (phrasePointers[7][0] + spaceForPointersAndInstrs) & 0xFF : 0xFB;
	allPointersAndInstrs[15 + add] = data[7].size() != 0 ? (phrasePointers[7][0] + spaceForPointersAndInstrs) >> 8 : 0xFF;

	if (hasIntro)
	{
		allPointersAndInstrs[16 + add] = data[0].size() != 0 ? (phrasePointers[0][1] + spaceForPointersAndInstrs) & 0xFF : 0xFB;
		allPointersAndInstrs[17 + add] = data[0].size() != 0 ? (phrasePointers[0][1] + spaceForPointersAndInstrs) >> 8 : 0xFF;
		allPointersAndInstrs[18 + add] = data[1].size() != 0 ? (phrasePointers[1][1] + spaceForPointersAndInstrs) & 0xFF : 0xFB;
		allPointersAndInstrs[19 + add] = data[1].size() != 0 ? (phrasePointers[1][1] + spaceForPointersAndInstrs) >> 8 : 0xFF;
		allPointersAndInstrs[20 + add] = data[2].size() != 0 ? (phrasePointers[2][1] + spaceForPointersAndInstrs) & 0xFF : 0xFB;
		allPointersAndInstrs[21 + add] = data[2].size() != 0 ? (phrasePointers[2][1] + spaceForPointersAndInstrs) >> 8 : 0xFF;
		allPointersAndInstrs[22 + add] = data[3].size() != 0 ? (phrasePointers[3][1] + spaceForPointersAndInstrs) & 0xFF : 0xFB;
		allPointersAndInstrs[23 + add] = data[3].size() != 0 ? (phrasePointers[3][1] + spaceForPointersAndInstrs) >> 8 : 0xFF;
		allPointersAndInstrs[24 + add] = data[4].size() != 0 ? (phrasePointers[4][1] + spaceForPointersAndInstrs) & 0xFF : 0xFB;
		allPointersAndInstrs[25 + add] = data[4].size() != 0 ? (phrasePointers[4][1] + spaceForPointersAndInstrs) >> 8 : 0xFF;
		allPointersAndInstrs[26 + add] = data[5].size() != 0 ? (phrasePointers[5][1] + spaceForPointersAndInstrs) & 0xFF : 0xFB;
		allPointersAndInstrs[27 + add] = data[5].size() != 0 ? (phrasePointers[5][1] + spaceForPointersAndInstrs) >> 8 : 0xFF;
		allPointersAndInstrs[28 + add] = data[6].size() != 0 ? (phrasePointers[6][1] + spaceForPointersAndInstrs) & 0xFF : 0xFB;
		allPointersAndInstrs[29 + add] = data[6].size() != 0 ? (phrasePointers[6][1] + spaceForPointersAndInstrs) >> 8 : 0xFF;
		allPointersAndInstrs[30 + add] = data[7].size() != 0 ? (phrasePointers[7][1] + spaceForPointersAndInstrs) & 0xFF : 0xFB;
		allPointersAndInstrs[31 + add] = data[7].size() != 0 ? (phrasePointers[7][1] + spaceForPointersAndInstrs) >> 8 : 0xFF;
	}

	totalSize = data[0].size() + data[1].size() + data[2].size() + data[3].size() + data[4].size() + data[5].size() + data[6].size() + data[7].size() + data[8].size() + spaceForPointersAndInstrs;



	//if (tempo == -1) tempo = 0x36;
	unsigned int totalLength;
	mainLength = -1;
	for (i = 0; i < 8; i++)
	if (channelLengths[i] != 0)
		mainLength = std::min(mainLength, (unsigned int)channelLengths[i]);
	if (mainLength == -1)
		throw AddmusicException("This song doesn't seem to have any data.", false, this);

		totalLength = mainLength;

	if (hasIntro)
		mainLength -= introLength;

	if (guessLength)
	{
		double l1 = 0, l2 = 0;
		bool onL1 = true;

		std::sort(tempoChanges.begin(), tempoChanges.end(), sortTempoPair);
		if (tempoChanges.size() == 0 || tempoChanges[0].first != 0)
		{
			tempoChanges.insert(tempoChanges.begin(), std::pair<double, int>(0, 0x36));			// Stick the default tempo at the beginning if necessary.
		}

		tempoChanges.push_back(std::pair<double, int>(totalLength, 0));			// Add in a dummy tempo change at the very end.

		for (size_t z = 0; z < tempoChanges.size() - 1; z++)
		{
			if (tempoChanges[z].first > totalLength)
			{
				throw AddmusicException("A tempo change was found beyond the end of the song.", false, this);
				break;
			}

			if (tempoChanges[z].second < 0)
				onL1 = false;

			double difference = tempoChanges[z + 1].first - tempoChanges[z].first;
			if (onL1)
				l1 += difference / (2 * std::abs(tempoChanges[z].second));
			else
				l2 += difference / (2 * std::abs(tempoChanges[z].second));

		}

		if (hasIntro)
		{
			seconds = (unsigned int)std::floor(l1 + l2 * 2 + 0.5);	// Just 2? Not 2.012584 or something?  Wow.
			mainSeconds = l2;
			introSeconds = l1;
		}
		else
		{
			mainSeconds = l1;
			seconds = (unsigned int)std::floor(l1 * 2 + 0.5);
		}
		knowsLength = true;
	}

	int spaceUsedBySamples = 0;
	for (i = 0; i < mySamples.size(); i++)
	{
		spaceUsedBySamples += 4 + samples[mySamples[i]].data.size();	// The 4 accounts for the space used by the SRCN table.
	}

	if (verbose)
		std::cout << name << " total size: 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << totalSize << " bytes" << std::dec << std::endl;
	else
		printChannelDataNonVerbose(totalSize);
	//for (int z = 0; z <= 8; z++)
	//{
	if (verbose)
	{
		printf("\t#0: 0x%03X #1: 0x%03X #2: 0x%03X #3: 0x%03X Ptrs+Instrs: 0x%03X\n\t#4: 0x%03X #5: 0x%03X #6: 0x%03X #7: 0x%03X Loop:        0x%03X \n", (unsigned int)data[0].size(), (unsigned int)data[1].size(), (unsigned int)data[2].size(), (unsigned int)data[3].size(), spaceForPointersAndInstrs, (unsigned int)data[4].size(), (unsigned int)data[5].size(), (unsigned int)data[6].size(), (unsigned int)data[7].size(), (unsigned int)data[8].size());

		printf("Space used by echo: 0x%04X bytes.  Space used by samples: 0x%04X bytes.\n\n", echoBufferSize << 11, spaceUsedBySamples);
	}
	//}
	if (totalSize > minSize && minSize > 0)
		std::cout << "File " << name << ", line " << line << ": Warning: Song was larger than it could pad out by 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << totalSize - minSize << " bytes." << std::dec << std::endl;

	std::stringstream statStrStream;

	statStrStream << "CHANNEL 0 SIZE:				0x" << hex4 << data[0].size() << "\n";
	statStrStream << "CHANNEL 1 SIZE:				0x" << hex4 << data[1].size() << "\n";
	statStrStream << "CHANNEL 2 SIZE:				0x" << hex4 << data[2].size() << "\n";
	statStrStream << "CHANNEL 3 SIZE:				0x" << hex4 << data[3].size() << "\n";
	statStrStream << "CHANNEL 4 SIZE:				0x" << hex4 << data[4].size() << "\n";
	statStrStream << "CHANNEL 5 SIZE:				0x" << hex4 << data[5].size() << "\n";
	statStrStream << "CHANNEL 6 SIZE:				0x" << hex4 << data[6].size() << "\n";
	statStrStream << "CHANNEL 7 SIZE:				0x" << hex4 << data[7].size() << "\n";
	statStrStream << "LOOP DATA SIZE:				0x" << hex4 << data[8].size() << "\n";
	statStrStream << "POINTERS AND INSTRUMENTS SIZE:		0x" << hex4 << spaceForPointersAndInstrs << "\n";
	statStrStream << "SAMPLES SIZE:				0x" << hex4 << spaceUsedBySamples << "\n";
	statStrStream << "ECHO SIZE:				0x" << hex4 << (echoBufferSize << 11) << "\n";
	statStrStream << "SONG TOTAL DATA SIZE:			0x" << hex4 << data[0].size() + data[1].size() + data[2].size() + data[3].size() + data[4].size() + data[5].size() + data[6].size() + data[7].size() + data[8].size() + spaceForPointersAndInstrs << "\n";
	if (index > highestGlobalSong)
		statStrStream << "FREE ARAM (APPROXIMATE):		0x" << hex4 << 0x10000 - (echoBufferSize << 11) - spaceUsedBySamples - totalSize - programUploadPos << "\n\n";
	else
		statStrStream << "FREE ARAM (APPROXIMATE):		UNKNOWN\n\n";
	statStrStream << "CHANNEL 0 TICKS:			0x" << hex4 << channelLengths[0] << "\n";
	statStrStream << "CHANNEL 1 TICKS:			0x" << hex4 << channelLengths[1] << "\n";
	statStrStream << "CHANNEL 2 TICKS:			0x" << hex4 << channelLengths[2] << "\n";
	statStrStream << "CHANNEL 3 TICKS:			0x" << hex4 << channelLengths[3] << "\n";
	statStrStream << "CHANNEL 4 TICKS:			0x" << hex4 << channelLengths[4] << "\n";
	statStrStream << "CHANNEL 5 TICKS:			0x" << hex4 << channelLengths[5] << "\n";
	statStrStream << "CHANNEL 6 TICKS:			0x" << hex4 << channelLengths[6] << "\n";
	statStrStream << "CHANNEL 7 TICKS:			0x" << hex4 << channelLengths[7] << "\n\n";
	if (knowsLength)
	{
		statStrStream << "SONG INTRO LENGTH IN SECONDS:		" << std::dec << introSeconds << "\n";
		statStrStream << "SONG MAIN LOOP LENGTH IN SECONDS:	" << mainSeconds << "\n";
		statStrStream << "SONG TOTAL LENGTH IN SECONDS:		" << introSeconds + mainSeconds << "\n";
	}
	else
	{
		statStrStream << "SONG INTRO LENGTH IN SECONDS:		UNKNOWN\n";
		statStrStream << "SONG MAIN LOOP LENGTH IN SECONDS:	UNKNOWN\n";
		statStrStream << "SONG TOTAL LENGTH IN SECONDS:		UNKNOWN\n";
	}

	statStr = statStrStream.str();

	std::string fname = name;

	int extPos = fname.find_last_of('.');
	if (extPos != -1)
		fname = fname.substr(0, extPos);

	if (fname.find('/') != -1)
		fname = fname.substr(fname.find_last_of('/') + 1);
	else if (fname.find('\\') != -1)
		fname = fname.substr(fname.find_last_of('\\') + 1);
	fname = "stats/" + fname + ".txt";

	writeTextFile(fname, statStr);
}

void Music::parseDefine()
{
	throw AddmusicException("A #define was found after the preprocessing stage.", false, this);
	//skipSpaces();
	//std::string defineName;
	//while (!isspace(text[pos]) && pos < text.length())
	//{
	//	defineName += text[pos++];
	//}

	//for (unsigned int z = 0; z < defineStrings.size(); z++)
	//	if (defineStrings[z] == defineName)
	//		throw AddmusicException("A string cannot be defined more than once.", false, this);

	//defineStrings.push_back(defineName);
}

void Music::parseUndef()
{
	throw AddmusicException("An #undef was found after the preprocessing stage.", false, this);
	//	skipSpaces();
	//	std::string defineName;
	//	while (!isspace(text[pos]) && pos < text.length())
	//	{
	//		defineName += text[pos++];
	//	}
	//	unsigned int z = -1;
	//	for (z = 0; z < defineStrings.size(); z++)
	//		if (defineStrings[z] == defineName)
	//			goto found;
	//
	//	throw AddmusicException("The specified string was never defined.", false, this);
	//
	//found:
	//
	//	defineStrings[z].clear();
}

void Music::parseIfdef()
{
	throw AddmusicException("An #ifdef was found after the preprocessing stage.", false, this);
	//	inDefineBlock = true;
	//	skipSpaces();
	//	std::string defineName;
	//	while (!isspace(text[pos]) && pos < text.length())
	//	{
	//		defineName += text[pos++];
	//	}
	//
	//	unsigned int z = -1;
	//
	//	int temp;
	//
	//	for (unsigned int z = 0; z < defineStrings.size(); z++)
	//		if (defineStrings[z] == defineName)
	//			goto found;
	//
	//	temp = text.find("#endif", pos);
	//
	//	if (temp == -1)
	//		throw AddmusicException("#ifdef was missing a matching #endif.", false, this);
	//
	//	pos = temp;
	//found:
	//	return;
}

void Music::parseIfndef()
{
	throw AddmusicException("An #ifndef was found after the preprocessing stage.", false, this);
	//	inDefineBlock = true;
	//	skipSpaces();
	//	std::string defineName;
	//	while (!isspace(text[pos]) && pos < text.length())
	//	{
	//		defineName += text[pos++];
	//	}
	//
	//	unsigned int z = -1;
	//
	//	for (unsigned int z = 0; z < defineStrings.size(); z++)
	//		if (defineStrings[z] == defineName)
	//			goto found;
	//
	//	return;
	//
	//found:
	//	int temp = text.find("#endif", pos);
	//
	//	if (temp == -1)
	//		throw AddmusicException("#ifdef was missing a matching #endif.", false, this);
	//
	//	pos = temp;
}

void Music::parseEndif()
{
	throw AddmusicException("An #endif was found after the preprocessing stage.", false, this);
	//if (inDefineBlock == false)
	//	throw AddmusicException("#endif was found without a matching #ifdef or #ifndef", false, this);
	//else
	//	inDefineBlock = false;
}

void Music::parseSPCInfo()
{
	skipSpaces();
	if (text[pos] != '{')
		throw AddmusicException("Could not find opening brace in SPC info command.", false, this);

	pos++;
	skipSpaces();

	while (text[pos] != '}')
	{
		if (text[pos] != '#')
			throw AddmusicException("Unexpected symbol found in SPC info command.  Expected \'#\'.", false, this);
		pos++;
		std::string typeName;

		while (!isspace(text[pos]))
			typeName += text[pos++];

		if (typeName != "author" && typeName != "comment" && typeName != "title" && typeName != "game" && typeName != "length")
		{
			throw AddmusicException("Unexpected type name found in SPC info command.  Only \"author\", \"comment\", \"title\", \"game\", and \"length\" are allowed.", false, this);
		}

		skipSpaces();

		if (text[pos] != '\"')
			throw AddmusicException("Unexpected symbol found in SPC info command.  Expected a quoted string.", false, this);

		pos++;
		int quotedStringLength = 0;
		std::string parameter = getQuotedString(text, pos, quotedStringLength);

		if (typeName == "author")
			author = parameter;
		else if (typeName == "comment")
			comment = parameter;
		else if (typeName == "title")
			title = parameter;
		else if (typeName == "game")
			game = parameter;
		else if (typeName == "length")
		{
			if (parameter == "auto")
			{
				guessLength = true;
			}
			else
			{
				guessLength = false;
				int p = 0;
				i = getInt(parameter, p);
				if (parameter[p] != ':' || i == -1)
					throw AddmusicException("Error parsing SPC length field.  Format must be m:ss or \"auto\".", false, this);

					p++;
				j = getInt(parameter, p);
				if (j == -1 || p != parameter.length())
					throw AddmusicException("Error parsing SPC length field.  Format must be m:ss or \"auto\".", false, this);

					seconds = i * 60 + j;

				if (seconds > 999)
					throw AddmusicException("Songs longer than 16:39 are not allowed by the SPC format.", false, this);

					seconds = seconds & 0xFFFFFF;
				knowsLength = true;

			}
		}

		pos += quotedStringLength + 1;

		skipSpaces();
	}

	if (author.length() > 32)
	{
		author = author.substr(0, 32);
		throw AddmusicException((std::string)("\"Author\" field was too long.  Truncating to \"") + author + "\".", false);
	}
	if (game.length() > 32)
	{
		game = game.substr(0, 32);
		throw AddmusicException((std::string)("\"Game\" field was too long.  Truncating to \"") + game + "\".", false);
	}
	if (comment.length() > 32)
	{
		comment = comment.substr(0, 32);
		throw AddmusicException((std::string)("\"Comment\" field was too long.  Truncating to \"") + comment + "\".", false);
	}
	if (title.length() > 32)
	{
		title = title.substr(0, 32);
		throw AddmusicException((std::string)("\"Title\" field was too long.  Truncating to \"") + title + "\".", false);
	}

	pos++;
}

void Music::handleNormalLoopEnter()
{
	normalLoopLength = 0;
	if (inE6Loop)					// We're entering a normal loop that's nested in a super loop
	{
		baseLoopIsNormal = false;
		baseLoopIsSuper = true;
		extraLoopIsNormal = true;
		extraLoopIsSuper = false;
	}
	else						// We're entering a normal loop that's not nested
	{
		baseLoopIsNormal = true;
		baseLoopIsSuper = false;
		extraLoopIsNormal = false;
		extraLoopIsSuper = false;
	}
}

void Music::handleSuperLoopEnter()
{
	superLoopLength = 0;
	inE6Loop = true;
	if (channel == 8)				// We're entering a super loop that's nested in a normal loop
	{
		baseLoopIsNormal = true;
		baseLoopIsSuper = false;
		extraLoopIsNormal = false;
		extraLoopIsSuper = true;
	}
	else						// We're entering a super loop that's not nested
	{
		baseLoopIsNormal = false;
		baseLoopIsSuper = true;
		extraLoopIsNormal = false;
		extraLoopIsSuper = false;
	}
}

void Music::handleNormalLoopExit(int loopCount)
{
	if (extraLoopIsNormal)				// We're leaving a normal loop that's nested in a super loop.
	{
		extraLoopIsNormal = false;
		extraLoopIsSuper = false;
		superLoopLength += normalLoopLength * loopCount;
	}
	else if (baseLoopIsNormal)			// We're leaving a normal loop that's not nested.
	{
		baseLoopIsNormal = false;
		baseLoopIsSuper = false;
		channelLengths[channel] += normalLoopLength * loopCount;
	}

	if (loopLabel > 0)
	{
		loopLengths[loopLabel] = normalLoopLength;
	}
}

void Music::handleSuperLoopExit(int loopCount)
{
	inE6Loop = false;
	if (extraLoopIsSuper)				// We're leaving a super loop that's nested in a normal loop.
	{
		extraLoopIsNormal = false;
		extraLoopIsSuper = false;
		normalLoopLength += superLoopLength * loopCount;
	}
	else if (baseLoopIsSuper)			// We're leaving a super loop that's not nested.
	{
		baseLoopIsNormal = false;
		baseLoopIsSuper = false;
		channelLengths[channel] += superLoopLength * loopCount;
	}
}

void Music::handleNormalLoopRemoteCall(int loopCount)
{
	if (loopLabel == 0)
		addNoteLength(normalLoopLength * loopCount);
	else
		addNoteLength(loopLengths[loopLabel] * loopCount);
}


void Music::addNoteLength(double ticks)
{
	if (extraLoopIsNormal)
	{
		normalLoopLength += ticks;
	}
	else if (extraLoopIsSuper)
	{
		superLoopLength += ticks;
	}
	else if (baseLoopIsNormal)
	{
		normalLoopLength += ticks;
	}
	else if (baseLoopIsSuper)
	{
		superLoopLength += ticks;
	}
	else
	{
		channelLengths[channel] += ticks;
	}
}

int Music::divideByTempoRatio(int value, bool fractionIsError)
{
	if (targetAMKVersion < 4 || tempoRatio == 1) {
		return value;
	}
	int temp = value / tempoRatio;
	if (value % tempoRatio != 0)
	{
		if (fractionIsError) {
			if (fractionNoteLengthWarning) {
				throw AddmusicException("Using the tempo ratio on this value would result in a fractional value.", false, this);
			}
			else {
				throw AddmusicException("Attempted to use a tempo ratio on a value that was already going to output a fractional value.", false, this);
			}
		}
		else
		{
			throw AddmusicException("The tempo ratio resulted in a fractional value.", false, this);
		}
	}

	return temp;
}

int Music::multiplyByTempoRatio(int value)
{

	int temp = value * tempoRatio;
	if (temp >= 256)
		throw AddmusicException("Using the tempo ratio on this value would cause it to overflow.", false, this);

	return temp;
}