/*
Copyright (C) 2018 Juraj Andrássy
repository https://github.com/jandrassy

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ChunkedPrint.h"

#define ENCODING_RESERVE 7

ChunkedPrint::ChunkedPrint(Print &target, uint8_t* buffer, size_t size)
  : BufferedPrint(target, buffer + ENCODING_RESERVE, size - ENCODING_RESERVE) {
}

ChunkedPrint::ChunkedPrint(Print &target, char* buffer, size_t size)
  : BufferedPrint(target, (uint8_t*) buffer + ENCODING_RESERVE, size - ENCODING_RESERVE) {
}

void ChunkedPrint::begin() {
  BufferedPrint::flush();
  encode = true;
  firstChunk = true;
}

void ChunkedPrint::end() {
  if (pos > 0) {
    flush();
  }
  if (encode) {
    target.println("\r\n0\r\n");
    encode = false;
  }
}

void ChunkedPrint::flush() {
  if (!encode) {
    BufferedPrint::flush();
    return;
  }
  if (!pos)
    return;
  uint8_t l = (firstChunk ? 2 : 4) + (pos > 0xFF ? 3 : (pos <= 0xF ? 1 : 2));
//  uint8_t l = 2 + (pos > 0xFF ? 3 : (pos <= 0xF ? 1 : 2));
  sprintf((char*) buffer - l, (firstChunk ? ("%x\r") : ("\r\n%x\r")), pos);
//  sprintf((char*) buffer - l, "%x\r", pos);
  (buffer - 1)[0] = '\n';
//  buffer[pos++] = '\r';
//  buffer[pos++] = '\n';
  if (firstChunk) {
    firstChunk = false;
  }
  target.write(buffer - l, l + pos);
  pos = 0;
}

