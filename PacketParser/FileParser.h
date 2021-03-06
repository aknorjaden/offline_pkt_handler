/*
	------------------------------------------------------------------------------------
	LICENSE:
	------------------------------------------------------------------------------------
	This file is part of EVEmu: EVE Online Server Emulator
	Copyright 2006 - 2011 The EVEmu Team
	For the latest information visit http://evemu.org
	------------------------------------------------------------------------------------
	This program is free software; you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free Software
	Foundation; either version 2 of the License, or (at your option) any later
	version.

	This program is distributed in the hope that it will be useful, but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License along with
	this program; if not, write to the Free Software Foundation, Inc., 59 Temple
	Place - Suite 330, Boston, MA 02111-1307, USA, or go to
	http://www.gnu.org/copyleft/lesser.txt.
	------------------------------------------------------------------------------------
	Author:		Captnoord
*/

#ifndef _FILE_PARSER_H
#define _FILE_PARSER_H

#include <stdio.h>

FILE* fp_in;
FILE* fp_out;

uint8 Getuint8()
{
	uint8 buff;
	fread(&buff,1,1,fp_in);
	return buff;
}

uint16 Getuint16()
{
	uint16 buff;
	fread(&buff,2,1,fp_in);
	return buff;
}

uint32 Getuint32()
{
	uint32 buff;
	fread(&buff,4,1,fp_in);
	return buff;
}

int32 Getint32()
{
	int32 buff;
	fread(&buff,4,1,fp_in);
	return buff;
}

uint64 Getuint64()
{
	uint64 buff;
	fread(&buff,8,1,fp_in);
	return buff;
}

char* GetBuffer(size_t len)
{
	char* buff = (char*)ASCENT_MALLOC(len);
	assert(buff);
	fread(buff, len,1,fp_in);
	return buff;
}

void HandleFile(const char* in_file_path, const char* out_file_path)
{
	std::string out_path;
	if (out_file_path == NULL)
	{
		out_path = in_file_path;
		out_path.resize(out_path.size() - 4);
		out_path+="_handled.txt";
	}
	else
	{
		out_path = out_file_path;
	}

	fp_in = fopen(in_file_path,"rb");
	fp_out = fopen(out_path.c_str(), "w");

	long fsize = 0;
	fseek(fp_in,0,SEEK_END);
	fsize = ftell(fp_in);
	fseek(fp_in,0,SEEK_SET);

	size_t i = 0;
	while (ftell(fp_in) < fsize)
	{
		uint32 FileOffset = ftell(fp_in);
		time_t timestamp = Getuint64();
		uint8 direction = Getuint8();
		int length = Getint32();
		char* packetBuf = GetBuffer(length);

		fprintf(fp_out, "\n{%s} FileOffset:0x%X, Packet Nr:%u, length:%u, time:%u\n", (direction ? "SERVER" : "CLIENT"), FileOffset, i, length, uint32(timestamp & 0xFFFFFFFF));
		
		ReadStream readstream(packetBuf, length);
        MarshalStream stream;
		PyObject* henk = stream.load(readstream);

		if (henk != NULL)
		{
#ifdef MARSHAL_REMARSHAL

			WriteStream writeStream(length);
			bool saveret = stream.save(henk, writeStream);

			if (saveret == false)
			{
				HexAsciiModule::print_hexview(stdout, (char*)packetBuf, length);
				DumpObject(stdout, henk);
				HexAsciiModule::print_hexview(stdout, (char*)writeStream.content(), writeStream.size());
				ASCENT_HARDWARE_BREAKPOINT;
			}

			size_t writeStreamSize = writeStream.size();

			//debug checking exception, client packets had a hash in front of them I am not including in the marshal write 
			if (direction == 0)
			{
				if (readstream.buffersize() != writeStream.size())
					writeStream.insert((uint8*)"\x1C\0\0\0\0", 5, 5);
			}
			else
			{
				// do the mem compare to check if the rebuild packets are the same 
				if(memcmp(writeStream.content(), readstream.content(), readstream.size()))
				{
					DumpObject(stdout, henk);
					HexAsciiModule::print_hexview(stdout, (char*)packetBuf, length);
					HexAsciiModule::print_hexview(stdout, (char*)writeStream.content(), writeStream.size());
					//ASCENT_HARDWARE_BREAKPOINT;
				}
			}

			if (readstream.buffersize() != writeStream.size())
			{
				DumpObject(stdout, henk);
				HexAsciiModule::print_hexview(stdout, (char*)packetBuf, length);
				HexAsciiModule::print_hexview(stdout, (char*)writeStream.content(), writeStream.size());
				//ASCENT_HARDWARE_BREAKPOINT;
			}
#endif//ENABLE_MARSHAL_SAVE_CHECK

			DumpObject(fp_out, henk);
			henk->DecRef();
		}

		ASCENT_FREE(packetBuf);
		
		if (readstream.tell()!= readstream.size())
			Log.Warning("OfflinePacketParser","sub stream isn't handled completely: %u - %u", readstream.tell(), readstream.size());

		//printf("Parsed packet nr: %u | offset: %u, size: %u\n", i, readstream.tell(), readstream.size());

		i++;
	}

	fclose(fp_out);
	fclose (fp_in);

	printf("\nOffline packet handling done: handled:%u packets\n", i);
}

#endif//_FILE_PARSER_H
