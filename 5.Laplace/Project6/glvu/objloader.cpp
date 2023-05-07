#include "objloader.h"
#include <algorithm>
#include <fstream>
using namespace std;

namespace{
const size_t WORD_BUFFER_LENGTH = 512;
std::string copyLine(const char* inBuf, const char* bufEnd)
{
    if (!inBuf)
        return std::string();

    const char* ptr = inBuf;
    while (ptr < bufEnd){
        if (*ptr == '\n' || *ptr == '\r')
            break;
        ++ptr;
    }

    // we must avoid the +1 in case the array is used up
    return std::string(inBuf, (size_t)(ptr - inBuf + ((ptr < bufEnd) ? 1 : 0)));
}

size_t copyWord(char* outBuf, const char* const inBuf, size_t outBufLength, const char* const bufEnd)
{
	if (!outBufLength)
		return 0;

	if (!inBuf) {
		*outBuf = 0;
		return 0;
	}

	size_t i = 0;
	while(inBuf[i]) {
		if (isspace(inBuf[i]) || &(inBuf[i]) == bufEnd)
			break;
		++i;
	}

	size_t length = std::min(i, outBufLength-1);
	for (size_t j=0; j<length; ++j)
		outBuf[j] = inBuf[j];

	outBuf[length] = 0;
	return length;
}

//! skip space characters and stop on first non-space
const char* goFirstWord(const char* buf, const char* const bufEnd, bool acrossNewlines=true)
{
	// skip space characters
	if (acrossNewlines)
		while((buf != bufEnd) && isspace(*buf))
			++buf;
	else
		while((buf != bufEnd) && isspace(*buf) && (*buf != '\n'))
			++buf;

	return buf;
}


//! skip current word and stop at beginning of next one
const char* goNextWord(const char* buf, const char* const bufEnd, bool acrossNewlines=true)
{
	// skip current word
	while(( buf != bufEnd ) && !isspace(*buf))
		++buf;

	return goFirstWord(buf, bufEnd, acrossNewlines);
}

const char* goAndCopyNextWord(char* outBuf, const char* inBuf, size_t outBufLength, const char* bufEnd)
{
	inBuf = goNextWord(inBuf, bufEnd, false);
	copyWord(outBuf, inBuf, outBufLength, bufEnd);
	return inBuf;
}

//! Read vector of floats
template<int dim>
const char* readVec(const char* bufPtr, float *vec, const char* const bufEnd)
{
	static char wordBuffer[WORD_BUFFER_LENGTH];

    for (int i = 0; i < dim; i++){
        bufPtr = goAndCopyNextWord(wordBuffer, bufPtr, WORD_BUFFER_LENGTH, bufEnd);
        vec[i] = float(atof(wordBuffer));
    }

	return bufPtr;
}

bool retrieveVertexIndices(const char* vertexData, int* idx, const char* bufEnd)
{
	const char* p = goFirstWord(vertexData, bufEnd);

	char word[16];
	size_t idxType = 0;	// 0 = posIdx, 1 = texcoordIdx, 2 = normalIdx

	size_t i = 0;
    while (p != bufEnd){
        if ((isdigit(*p)) || (*p == '-')){
			// build up the number
			word[i++] = *p;
		}
        else if (*p == '/' || *p == ' ' || *p == '\0'){
			// number is completed. Convert and store it
			word[i] = '\0';
			// if no number was found index will become 0 and later on -1 by decrement
			idx[idxType] = strtol(word, nullptr, 10);

            if (idx[idxType] < 0)
                fprintf(stderr, "relative indexing not implemented!\n");
			else
				idx[idxType]-=1;

			// reset the word
			word[0] = '\0';
			i = 0;

			// go to the next kind of index type
            if (*p == '/'){
                if (++idxType > 2){
					// error checking, shouldn't reach here unless file is wrong
					idxType = 0;
				}
			}
            else{
				// set all missing values to disable (=-1)
				while (++idxType < 3)
					idx[idxType]=-1;
				++p;
				break; // while
			}
		}

		// go to the next char
		++p;
	}

	return true;
}

size_t countSubStr(const std::string & str,  const std::string & obj) 
{
    size_t n = 0;
    size_t pos = 0;
    while ((pos = obj.find(str, pos)) != std::string::npos) {
        n++;
        pos += str.size();
    }
    return n;
}


//! Read until line break is reached and stop at the next non-space character
const char* goNextLine(const char* buf, const char* const bufEnd)
{
    // look for newline characters
    while (buf != bufEnd){
        // found it, so leave
        if (*buf == '\n' || *buf == '\r')
            break;
        ++buf;
    }
    return goFirstWord(buf, bufEnd);
}

std::string readFile2Str(const char* filename)
{
    FILE* fp = fopen(filename, "r");  //open the file
    if (!fp) {
        fprintf(stderr, "!error opening file %s!!!\n", filename);
        return std::string();
    }

    fseek(fp, 0, SEEK_END);
    int len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    std::string str(len, '0');
    setvbuf(fp, nullptr, _IONBF, 0); // solve the following fread problem
    len = fread(&str[0], 1, len, fp);
    fclose(fp);
	if(len<str.size())	str.resize(len);
    return str;
}

} //anonymous namespace

bool readObj(const char *filename, std::vector<float> &vertices, std::vector<int> &faces)
{
    std::string buf = readFile2Str(filename);
	const int filesize = buf.length();
	if (!filesize)
		return false;

	// Process obj information
    const char* bufPtr = buf.c_str();
    const char* bufEnd = bufPtr + filesize;

    int nv = countSubStr("\nv ", buf);
    vertices.reserve(nv*3);

    int nf = countSubStr("\nf ", buf);
    faces.reserve(nf * 3);

    while (bufPtr != bufEnd){
        switch (bufPtr[0]){
        case 'm':
            break;

		case 'v':               // v, vn, vt
            switch (bufPtr[1]){
			case ' ':{          // vertex
					float vec[3];
					bufPtr = readVec<3>(bufPtr, vec, bufEnd);
					vertices.insert(vertices.end(), vec, vec+3);
				}
				break;

			case 'n':
            case 't':
				break;
			}
			break;

		case 'o':  // object name
		case 'g':
		case 's':
        case 'u':
			break;

		case 'f':{               // face
			char vertexWord[WORD_BUFFER_LENGTH]; // for retrieving vertex data

			// get all vertices data in this face (current line of obj file)
			const std::string wordBuffer = copyLine(bufPtr, bufEnd);
			const char* linePtr = wordBuffer.c_str();
			const char* const endPtr = linePtr+wordBuffer.size();

            std::vector<int> faceCorners;
			faceCorners.reserve(10);            // should be large enough

			// read in all vertices
			linePtr = goNextWord(linePtr, endPtr);
            while (0 != linePtr[0]){
				// Array to communicate with retrieveVertexIndices()
				// sends the buffer sizes and gets the actual indices
				// if index not set returns -1
                int Idx[3] = { -1, -1, -1 };

				// read in next vertex's data
				size_t wlength = copyWord(vertexWord, linePtr, WORD_BUFFER_LENGTH, endPtr);
				// this function will also convert obj's 1-based index to c++'s 0-based index
				retrieveVertexIndices(vertexWord, Idx, vertexWord+wlength+1);

				faceCorners.push_back(Idx[0]);

				// go to next vertex
				linePtr = goNextWord(linePtr, endPtr);
			}

			// triangulate the face
            for (size_t i = 1; i < faceCorners.size() - 1; ++i){
				// Add a triangle
                faces.insert(faces.end(), { faceCorners[0], faceCorners[i], faceCorners[i + 1] });
			}
		}
		break;

		case '#': // comment
		default:
			break;
		}	// end switch(bufPtr[0])
		// eat up rest of line
		bufPtr = goNextLine(bufPtr, bufEnd);
	}

    return true;
}

