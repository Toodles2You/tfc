//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Whatever
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include <time.h>
#include "shake.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "UserMessages.h"

// --------------------------------------------------------------
//
// CSave
//
// --------------------------------------------------------------
static int gSizes[FIELD_TYPECOUNT] =
	{
		sizeof(float),	   // FIELD_FLOAT
		sizeof(int),	   // FIELD_STRING
		sizeof(int),	   // FIELD_ENTITY
		sizeof(int),	   // FIELD_CLASSPTR
		sizeof(EHANDLE),   // FIELD_EHANDLE
		sizeof(int),	   // FIELD_EVARS
		sizeof(int),	   // FIELD_EDICT
		sizeof(float) * 3, // FIELD_VECTOR
		sizeof(float) * 3, // FIELD_POSITION_VECTOR
		sizeof(int*),	   // FIELD_POINTER
		sizeof(int),	   // FIELD_INTEGER
#ifdef __GNUC__
		sizeof(int*) * 2, // FIELD_FUNCTION
#else
		sizeof(int*), // FIELD_FUNCTION
#endif
		sizeof(byte),		   // FIELD_BOOLEAN
		sizeof(short),		   // FIELD_SHORT
		sizeof(char),		   // FIELD_CHARACTER
		sizeof(float),		   // FIELD_TIME
		sizeof(int),		   // FIELD_MODELNAME
		sizeof(int),		   // FIELD_SOUNDNAME
		sizeof(std::uint64_t), //FIELD_INT64
};


// Base class includes common SAVERESTOREDATA pointer, and manages the entity table
CSaveRestoreBuffer::CSaveRestoreBuffer(SAVERESTOREDATA& data)
	: m_data(data)
{
}


CSaveRestoreBuffer::~CSaveRestoreBuffer() = default;

int CSaveRestoreBuffer::EntityIndex(CBaseEntity* pEntity)
{
	if (pEntity == nullptr)
		return -1;
	return EntityIndex(&pEntity->v);
}

int CSaveRestoreBuffer::EntityIndex(EntityOffset eoLookup)
{
	return EntityIndex(ENT(eoLookup));
}


int CSaveRestoreBuffer::EntityIndex(Entity* pentLookup)
{
	if (pentLookup == nullptr)
		return -1;

	int i;
	ENTITYTABLE* pTable;

	for (i = 0; i < m_data.tableCount; i++)
	{
		pTable = m_data.pTable + i;
		if (pTable->pent == pentLookup)
			return i;
	}
	return -1;
}


Entity* CSaveRestoreBuffer::EntityFromIndex(int entityIndex)
{
	if (entityIndex < 0)
		return nullptr;

	int i;
	ENTITYTABLE* pTable;

	for (i = 0; i < m_data.tableCount; i++)
	{
		pTable = m_data.pTable + i;
		if (pTable->id == entityIndex)
			return pTable->pent;
	}
	return nullptr;
}


int CSaveRestoreBuffer::EntityFlagsSet(int entityIndex, int flags)
{
	if (entityIndex < 0)
		return 0;
	if (entityIndex > m_data.tableCount)
		return 0;

	m_data.pTable[entityIndex].flags |= flags;

	return m_data.pTable[entityIndex].flags;
}


void CSaveRestoreBuffer::BufferRewind(int size)
{
	if (m_data.size < size)
		size = m_data.size;

	m_data.pCurrentData -= size;
	m_data.size -= size;
}

#ifndef WIN32
extern "C" {
unsigned _rotr(unsigned val, int shift)
{
	unsigned lobit;	 /* non-zero means lo bit set */
	unsigned num = val; /* number to rotate */

	shift &= 0x1f; /* modulo 32 -- this will also make
										   negative shifts work */

	while (shift--)
	{
		lobit = num & 1; /* get high bit */
		num >>= 1;		 /* shift right one bit */
		if (lobit)
			num |= 0x80000000; /* set hi bit if lo bit was set */
	}

	return num;
}
}
#endif

unsigned int CSaveRestoreBuffer::HashString(const char* pszToken)
{
	unsigned int hash = 0;

	while ('\0' != *pszToken)
		hash = _rotr(hash, 4) ^ *pszToken++;

	return hash;
}

unsigned short CSaveRestoreBuffer::TokenHash(const char* pszToken)
{
#ifndef NDEBUG
	static int tokensparsed = 0;
	tokensparsed++;
#endif
	if (0 == m_data.tokenCount || nullptr == m_data.pTokens)
	{
		//if we're here it means trigger_changelevel is trying to actually save something when it's not supposed to.
		engine::AlertMessage(at_error, "No token table array in TokenHash()!\n");
		return 0;
	}

	const unsigned short hash = (unsigned short)(HashString(pszToken) % (unsigned)m_data.tokenCount);

	for (int i = 0; i < m_data.tokenCount; i++)
	{
#ifndef NDEBUG
		static bool beentheredonethat = false;
		if (i > 50 && !beentheredonethat)
		{
			beentheredonethat = true;
			engine::AlertMessage(at_error, "CSaveRestoreBuffer :: TokenHash() is getting too full!\n");
		}
#endif

		int index = hash + i;
		if (index >= m_data.tokenCount)
			index -= m_data.tokenCount;

		if (!m_data.pTokens[index] || strcmp(pszToken, m_data.pTokens[index]) == 0)
		{
			m_data.pTokens[index] = (char*)pszToken;
			return index;
		}
	}

	// Token hash table full!!!
	// [Consider doing overflow table(s) after the main table & limiting linear hash table search]
	engine::AlertMessage(at_error, "CSaveRestoreBuffer :: TokenHash() is COMPLETELY FULL!\n");
	return 0;
}

void CSave::WriteData(const char* pname, int size, const char* pdata)
{
	BufferField(pname, size, pdata);
}


void CSave::WriteShort(const char* pname, const short* data, int count)
{
	BufferField(pname, sizeof(short) * count, (const char*)data);
}


void CSave::WriteInt(const char* pname, const int* data, int count)
{
	BufferField(pname, sizeof(int) * count, (const char*)data);
}


void CSave::WriteFloat(const char* pname, const float* data, int count)
{
	BufferField(pname, sizeof(float) * count, (const char*)data);
}


void CSave::WriteTime(const char* pname, const float* data, int count)
{
	int i;
	Vector tmp, input;

	BufferHeader(pname, sizeof(float) * count);
	for (i = 0; i < count; i++)
	{
		float tmp = data[0];

		// Always encode time as a delta from the current time so it can be re-based if loaded in a new level
		// Times of 0 are never written to the file, so they will be restored as 0, not a relative time
		tmp -= m_data.time;

		BufferData((const char*)&tmp, sizeof(float));
		data++;
	}
}


void CSave::WriteString(const char* pname, const char* pdata)
{
#ifdef TOKENIZE
	short token = (short)TokenHash(pdata);
	WriteShort(pname, &token, 1);
#else
	BufferField(pname, strlen(pdata) + 1, pdata);
#endif
}


void CSave::WriteString(const char* pname, const int* stringId, int count)
{
	int i, size;

#ifdef TOKENIZE
	short token = (short)TokenHash(STRING(*stringId));
	WriteShort(pname, &token, 1);
#else
#if 0
	if ( count != 1 )
		engine::AlertMessage( at_error, "No string arrays!\n" );
	WriteString( pname, (char *)STRING(*stringId) );
#endif

	size = 0;
	for (i = 0; i < count; i++)
		size += strlen(STRING(stringId[i])) + 1;

	BufferHeader(pname, size);
	for (i = 0; i < count; i++)
	{
		const char* pString = STRING(stringId[i]);
		BufferData(pString, strlen(pString) + 1);
	}
#endif
}


void CSave::WriteVector(const char* pname, const Vector& value)
{
	WriteVector(pname, &value.x, 1);
}


void CSave::WriteVector(const char* pname, const float* value, int count)
{
	BufferHeader(pname, sizeof(float) * 3 * count);
	BufferData((const char*)value, sizeof(float) * 3 * count);
}



void CSave::WritePositionVector(const char* pname, const Vector& value)
{
	if (0 != m_data.fUseLandmark)
	{
		Vector tmp = value - m_data.vecLandmarkOffset;
		WriteVector(pname, tmp);
	}

	WriteVector(pname, value);
}


void CSave::WritePositionVector(const char* pname, const float* value, int count)
{
	int i;
	Vector tmp, input;

	BufferHeader(pname, sizeof(float) * 3 * count);
	for (i = 0; i < count; i++)
	{
		Vector tmp(value[0], value[1], value[2]);

		if (0 != m_data.fUseLandmark)
			tmp = tmp - m_data.vecLandmarkOffset;

		BufferData((const char*)&tmp.x, sizeof(float) * 3);
		value += 3;
	}
}


void CSave::WriteFunction(const char* pname, void** data, int count)
{
	const char* functionName;

	functionName = engine::NameForFunction((uint32)*data);
	if (functionName)
		BufferField(pname, strlen(functionName) + 1, functionName);
	else
		engine::AlertMessage(at_error, "Invalid function pointer in entity!\n");
}



bool CSave::WriteEntVars(const char* pname, Entity* entity)
{
	return WriteFields(pname, entity, CBaseEntity::m_EntvarsDescription, ARRAYSIZE(CBaseEntity::m_EntvarsDescription));
}



bool CSave::WriteFields(const char* pname, void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount)
{
	int i, j, actualCount, emptyCount;
	TYPEDESCRIPTION* pTest;
	int entityArray[MAX_ENTITYARRAY];
	byte boolArray[MAX_ENTITYARRAY];

	// Precalculate the number of empty fields
	emptyCount = 0;
	for (i = 0; i < fieldCount; i++)
	{
		void* pOutputData;
		pOutputData = ((char*)pBaseData + pFields[i].fieldOffset);
		if (DataEmpty((const char*)pOutputData, pFields[i].fieldSize * gSizes[pFields[i].fieldType]))
			emptyCount++;
	}

	// Empty fields will not be written, write out the actual number of fields to be written
	actualCount = fieldCount - emptyCount;
	WriteInt(pname, &actualCount, 1);

	for (i = 0; i < fieldCount; i++)
	{
		void* pOutputData;
		pTest = &pFields[i];
		pOutputData = ((char*)pBaseData + pTest->fieldOffset);

		// UNDONE: Must we do this twice?
		if (DataEmpty((const char*)pOutputData, pTest->fieldSize * gSizes[pTest->fieldType]))
			continue;

		switch (pTest->fieldType)
		{
		case FIELD_FLOAT:
			WriteFloat(pTest->fieldName, (float*)pOutputData, pTest->fieldSize);
			break;
		case FIELD_TIME:
			WriteTime(pTest->fieldName, (float*)pOutputData, pTest->fieldSize);
			break;
		case FIELD_MODELNAME:
		case FIELD_SOUNDNAME:
		case FIELD_STRING:
			WriteString(pTest->fieldName, (int*)pOutputData, pTest->fieldSize);
			break;
		case FIELD_CLASSPTR:
		case FIELD_EVARS:
		case FIELD_EDICT:
		case FIELD_ENTITY:
		case FIELD_EHANDLE:
			if (pTest->fieldSize > MAX_ENTITYARRAY)
				engine::AlertMessage(at_error, "Can't save more than %d entities in an array!!!\n", MAX_ENTITYARRAY);
			for (j = 0; j < pTest->fieldSize; j++)
			{
				switch (pTest->fieldType)
				{
				case FIELD_EVARS:
					entityArray[j] = EntityIndex(((Entity**)pOutputData)[j]);
					break;
				case FIELD_CLASSPTR:
					entityArray[j] = EntityIndex(((CBaseEntity**)pOutputData)[j]);
					break;
				case FIELD_EDICT:
					entityArray[j] = EntityIndex(((Entity**)pOutputData)[j]);
					break;
				case FIELD_ENTITY:
					entityArray[j] = EntityIndex(((EntityOffset*)pOutputData)[j]);
					break;
				case FIELD_EHANDLE:
					entityArray[j] = EntityIndex((CBaseEntity*)(((EHANDLE*)pOutputData)[j]));
					break;
				}
			}
			WriteInt(pTest->fieldName, entityArray, pTest->fieldSize);
			break;
		case FIELD_POSITION_VECTOR:
			WritePositionVector(pTest->fieldName, pOutputData, pTest->fieldSize);
			break;
		case FIELD_VECTOR:
			WriteVector(pTest->fieldName, pOutputData, pTest->fieldSize);
			break;

		case FIELD_BOOLEAN:
		{
			//TODO: need to refactor save game stuff to make this cleaner and reusable
			//Convert booleans to bytes
			for (j = 0; j < pTest->fieldSize; j++)
			{
				boolArray[j] = ((bool*)pOutputData)[j] ? 1 : 0;
			}

			WriteData(pTest->fieldName, pTest->fieldSize, (char*)boolArray);
		}
		break;

		case FIELD_INTEGER:
			WriteInt(pTest->fieldName, (int*)pOutputData, pTest->fieldSize);
			break;

		case FIELD_INT64:
			WriteData(pTest->fieldName, sizeof(std::uint64_t) * pTest->fieldSize, ((char*)pOutputData));
			break;

		case FIELD_SHORT:
			WriteData(pTest->fieldName, 2 * pTest->fieldSize, ((char*)pOutputData));
			break;

		case FIELD_CHARACTER:
			WriteData(pTest->fieldName, pTest->fieldSize, ((char*)pOutputData));
			break;

		// For now, just write the address out, we're not going to change memory while doing this yet!
		case FIELD_POINTER:
			WriteInt(pTest->fieldName, (int*)(char*)pOutputData, pTest->fieldSize);
			break;

		case FIELD_FUNCTION:
			WriteFunction(pTest->fieldName, (void**)pOutputData, pTest->fieldSize);
			break;
		default:
			engine::AlertMessage(at_error, "Bad field type\n");
		}
	}

	return true;
}


void CSave::BufferString(char* pdata, int len)
{
	char c = 0;

	BufferData(pdata, len); // Write the string
	BufferData(&c, 1);		// Write a null terminator
}


bool CSave::DataEmpty(const char* pdata, int size)
{
	for (int i = 0; i < size; i++)
	{
		if (0 != pdata[i])
			return false;
	}
	return true;
}


void CSave::BufferField(const char* pname, int size, const char* pdata)
{
	BufferHeader(pname, size);
	BufferData(pdata, size);
}


void CSave::BufferHeader(const char* pname, int size)
{
	short hashvalue = TokenHash(pname);
	if (size > 1 << (sizeof(short) * 8))
		engine::AlertMessage(at_error, "CSave :: BufferHeader() size parameter exceeds 'short'!\n");
	BufferData((const char*)&size, sizeof(short));
	BufferData((const char*)&hashvalue, sizeof(short));
}


void CSave::BufferData(const char* pdata, int size)
{
	if (m_data.size + size > m_data.bufferSize)
	{
		engine::AlertMessage(at_error, "Save/Restore overflow!\n");
		m_data.size = m_data.bufferSize;
		return;
	}

	memcpy(m_data.pCurrentData, pdata, size);
	m_data.pCurrentData += size;
	m_data.size += size;
}



// --------------------------------------------------------------
//
// CRestore
//
// --------------------------------------------------------------

int CRestore::ReadField(void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount, int startField, int size, char* pName, void* pData)
{
	int i, j, stringCount, fieldNumber, entityIndex;
	TYPEDESCRIPTION* pTest;
	float timeData;
	Vector position;
	Entity* pent;
	char* pString;

	position = Vector(0, 0, 0);

	if (0 != m_data.fUseLandmark)
		position = m_data.vecLandmarkOffset;

	for (i = 0; i < fieldCount; i++)
	{
		fieldNumber = (i + startField) % fieldCount;
		pTest = &pFields[fieldNumber];
		if (!stricmp(pTest->fieldName, pName))
		{
			if (!m_global || (pTest->flags & FTYPEDESC_GLOBAL) == 0)
			{
				for (j = 0; j < pTest->fieldSize; j++)
				{
					void* pOutputData = ((char*)pBaseData + pTest->fieldOffset + (j * gSizes[pTest->fieldType]));
					void* pInputData = (char*)pData + j * gSizes[pTest->fieldType];

					switch (pTest->fieldType)
					{
					case FIELD_TIME:
						timeData = *(float*)pInputData;
						// Re-base time variables
						timeData += m_data.time;
						*((float*)pOutputData) = timeData;
						break;
					case FIELD_FLOAT:
						*((float*)pOutputData) = *(float*)pInputData;
						break;
					case FIELD_MODELNAME:
					case FIELD_SOUNDNAME:
					case FIELD_STRING:
						// Skip over j strings
						pString = (char*)pData;
						for (stringCount = 0; stringCount < j; stringCount++)
						{
							while ('\0' != *pString)
								pString++;
							pString++;
						}
						pInputData = pString;
						if (strlen((char*)pInputData) == 0)
							*((int*)pOutputData) = 0;
						else
						{
							int string;

							string = engine::AllocString((char*)pInputData);

							*((int*)pOutputData) = string;

							if (!FStringNull(string) && m_precache)
							{
								if (pTest->fieldType == FIELD_MODELNAME)
									engine::PrecacheModel((char*)STRING(string));
								else if (pTest->fieldType == FIELD_SOUNDNAME)
									engine::PrecacheSound((char*)STRING(string));
							}
						}
						break;
					case FIELD_EVARS:
						entityIndex = *(int*)pInputData;
						pent = EntityFromIndex(entityIndex);
						if (pent)
							*((Entity**)pOutputData) = pent;
						else
							*((Entity**)pOutputData) = nullptr;
						break;
					case FIELD_CLASSPTR:
						entityIndex = *(int*)pInputData;
						pent = EntityFromIndex(entityIndex);
						if (pent)
							*((CBaseEntity**)pOutputData) = pent->Get<CBaseEntity>();
						else
							*((CBaseEntity**)pOutputData) = nullptr;
						break;
					case FIELD_EDICT:
						entityIndex = *(int*)pInputData;
						pent = EntityFromIndex(entityIndex);
						*((Entity**)pOutputData) = pent;
						break;
					case FIELD_EHANDLE:
						// Input and Output sizes are different!
						pInputData = (char*)pData + j * sizeof(int);
						entityIndex = *(int*)pInputData;
						pent = EntityFromIndex(entityIndex);
						if (pent)
							*((EHANDLE*)pOutputData) = pent->Get<CBaseEntity>();
						else
							*((EHANDLE*)pOutputData) = nullptr;
						break;
					case FIELD_ENTITY:
						entityIndex = *(int*)pInputData;
						pent = EntityFromIndex(entityIndex);
						if (pent)
							*((EntityOffset*)pOutputData) = OFFSET(pent);
						else
							*((EntityOffset*)pOutputData) = 0;
						break;
					case FIELD_VECTOR:
						((float*)pOutputData)[0] = ((float*)pInputData)[0];
						((float*)pOutputData)[1] = ((float*)pInputData)[1];
						((float*)pOutputData)[2] = ((float*)pInputData)[2];
						break;
					case FIELD_POSITION_VECTOR:
						((float*)pOutputData)[0] = ((float*)pInputData)[0] + position.x;
						((float*)pOutputData)[1] = ((float*)pInputData)[1] + position.y;
						((float*)pOutputData)[2] = ((float*)pInputData)[2] + position.z;
						break;

					case FIELD_BOOLEAN:
					{
						// Input and Output sizes are different!
						pOutputData = (char*)pOutputData + j * (sizeof(bool) - gSizes[pTest->fieldType]);
						const bool value = *((byte*)pInputData) != 0;

						*((bool*)pOutputData) = value;
					}
					break;

					case FIELD_INTEGER:
						*((int*)pOutputData) = *(int*)pInputData;
						break;

					case FIELD_INT64:
						*((std::uint64_t*)pOutputData) = *(std::uint64_t*)pInputData;
						break;

					case FIELD_SHORT:
						*((short*)pOutputData) = *(short*)pInputData;
						break;

					case FIELD_CHARACTER:
						*((char*)pOutputData) = *(char*)pInputData;
						break;

					case FIELD_POINTER:
						*((int*)pOutputData) = *(int*)pInputData;
						break;
					case FIELD_FUNCTION:
						if (strlen((char*)pInputData) == 0)
							*((int*)pOutputData) = 0;
						else
							*((int*)pOutputData) = engine::FunctionFromName((char*)pInputData);
						break;

					default:
						engine::AlertMessage(at_error, "Bad field type\n");
					}
				}
			}
#if 0
			else
			{
				engine::AlertMessage( at_console, "Skipping global field %s\n", pName );
			}
#endif
			return fieldNumber;
		}
	}

	return -1;
}


bool CRestore::ReadEntVars(const char* pname, Entity* entity)
{
	return ReadFields(pname, entity, CBaseEntity::m_EntvarsDescription, ARRAYSIZE(CBaseEntity::m_EntvarsDescription));
}


bool CRestore::ReadFields(const char* pname, void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount)
{
	unsigned short i, token;
	int lastField, fileCount;
	HEADER header;

	i = ReadShort();

	token = ReadShort();

	// Check the struct name
	if (token != TokenHash(pname)) // Field Set marker
	{
		//		engine::AlertMessage( at_error, "Expected %s found %s!\n", pname, BufferPointer() );
		BufferRewind(2 * sizeof(short));
		return false;
	}

	// Skip over the struct name
	fileCount = ReadInt(); // Read field count

	lastField = 0; // Make searches faster, most data is read/written in the same order

	// Clear out base data
	for (i = 0; i < fieldCount; i++)
	{
		// Don't clear global fields
		if (!m_global || (pFields[i].flags & FTYPEDESC_GLOBAL) == 0)
			memset(((char*)pBaseData + pFields[i].fieldOffset), 0, pFields[i].fieldSize * gSizes[pFields[i].fieldType]);
	}

	for (i = 0; i < fileCount; i++)
	{
		BufferReadHeader(&header);
		lastField = ReadField(pBaseData, pFields, fieldCount, lastField, header.size, m_data.pTokens[header.token], header.pData);
		lastField++;
	}

	return true;
}


void CRestore::BufferReadHeader(HEADER* pheader)
{
	pheader->size = ReadShort();	  // Read field size
	pheader->token = ReadShort();	  // Read field name token
	pheader->pData = BufferPointer(); // Field Data is next
	BufferSkipBytes(pheader->size);	  // Advance to next field
}


short CRestore::ReadShort()
{
	short tmp = 0;

	BufferReadBytes((char*)&tmp, sizeof(short));

	return tmp;
}

int CRestore::ReadInt()
{
	int tmp = 0;

	BufferReadBytes((char*)&tmp, sizeof(int));

	return tmp;
}

int CRestore::ReadNamedInt(const char* pName)
{
	HEADER header;

	BufferReadHeader(&header);
	return ((int*)header.pData)[0];
}

char* CRestore::ReadNamedString(const char* pName)
{
	HEADER header;

	BufferReadHeader(&header);
#ifdef TOKENIZE
	return (char*)(m_pdata->pTokens[*(short*)header.pData]);
#else
	return (char*)header.pData;
#endif
}


char* CRestore::BufferPointer()
{
	return m_data.pCurrentData;
}

void CRestore::BufferReadBytes(char* pOutput, int size)
{
	if (Empty())
		return;

	if ((m_data.size + size) > m_data.bufferSize)
	{
		engine::AlertMessage(at_error, "Restore overflow!\n");
		m_data.size = m_data.bufferSize;
		return;
	}

	if (pOutput)
		memcpy(pOutput, m_data.pCurrentData, size);
	m_data.pCurrentData += size;
	m_data.size += size;
}


void CRestore::BufferSkipBytes(int bytes)
{
	BufferReadBytes(nullptr, bytes);
}

int CRestore::BufferSkipZString()
{
	const int maxLen = m_data.bufferSize - m_data.size;

	int len = 0;
	char* pszSearch = m_data.pCurrentData;
	while ('\0' != *pszSearch++ && len < maxLen)
		len++;

	len++;

	BufferSkipBytes(len);

	return len;
}

bool CRestore::BufferCheckZString(const char* string)
{
	const int maxLen = m_data.bufferSize - m_data.size;
	const int len = strlen(string);
	if (len <= maxLen)
	{
		if (0 == strncmp(string, m_data.pCurrentData, len))
			return true;
	}
	return false;
}
