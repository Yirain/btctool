#include <stdlib.h>
#include <string.h>
#include "../common.h"
#include "../codec/codec.h"
#include "../container/CStack.h"
#include "../container/CLinkedlist.h"
#include "script.h"

Script * new_Script()
{
	Script *new = (Script *)calloc(1, sizeof(Script));
	if (new == NULL)
		return NULL;

	new->script = new_CLinkedlist();
	if (new->script == NULL)
	{
		free(new);
		return NULL;
	}
	else
	{
		new->add_opcode = &Script_add_opcode;
		new->add_data   = &Script_add_data;
		new->to_string  = &Script_to_string;
		new->to_bytes   = &Script_to_bytes;
		return new;
	}
}

Script * new_Script_from_bytes(BYTE *bytes, size_t size)
{
	Script *new = new_Script();
	uint32_t pos = 0;

while (true) // Can't use break and continue to control the loop if I use switch-case.
{
// Check if opcode.
	if (is_opcode(bytes[pos]))
	{
		// Check if PUSHDATA1.
		if (bytes[pos] == OP_PUSHDATA1)
		{
			// Check if enough bytes remain.
			uint32_t remain = size-pos-1;
			uint32_t length = bytes[pos+1];
			if (remain < length)
				return NULL;

			// Add opcode.
			opcode *op = (opcode *)calloc(1, sizeof(opcode));
			if (op == NULL)
				return NULL;
			*op = bytes[pos];
			new->add_opcode(new, op);

			// Add data.
			BYTE *data = (BYTE *)calloc(length + 1, sizeof(BYTE));
			if (data == NULL)
				return NULL;
			data[0] = bytes[pos+1];
			for (uint32_t j = 0; j < length; ++j)
				data[j+1] = bytes[pos + 2 + j];
			new->add_data(new, data, length + 1);

			// Shift pos, break the loop if pos at the end.
			pos = pos + 1 + length + 1;
			if (pos == size)
				break;
			else continue;
		}

		// Check if PUSHDATA2.
		else if (bytes[pos] == OP_PUSHDATA2)
		{
			// Check if enough bytes remain.
			uint32_t remain = size-pos-1;
			uint32_t length = bytes[pos+1];
			         length = (length << 8) + bytes[pos+2];
			if (remain < length)
				return NULL;

			// Add opcode.
			opcode *op = (opcode *)calloc(1, sizeof(opcode));
			if (op == NULL)
				return NULL;
			*op = bytes[pos];
			new->add_opcode(new, op);

			// Add data.
			BYTE *data = (BYTE *)calloc(length + 2, sizeof(BYTE));
			if (data == NULL)
				return NULL;
			data[0] = bytes[pos+1];
			data[1] = bytes[pos+2];
			for (uint32_t j = 0; j < length; ++j)
				data[j+2] = bytes[pos + 3 + j];
			new->add_data(new, data, length + 2);

			// Shift pos, break the loop if pos at the end.
			pos = pos + 2 + length + 1;
			if (pos == size)
				break;
			else continue;
		}

		// Check if PUSHDATA4.
		else if (bytes[pos] == OP_PUSHDATA4)
		{
			// Check if enough bytes remain.
			uint32_t remain = size-pos-1;
			uint32_t length = bytes[pos+1];
			         length = (length << 8) + bytes[pos+2];
			         length = (length << 8) + bytes[pos+3];
			         length = (length << 8) + bytes[pos+4];
			if (remain < length)
				return NULL;

			// Add opcode.
			opcode *op = (opcode *)calloc(1, sizeof(opcode));
			if (op == NULL)
				return NULL;
			*op = bytes[pos];
			new->add_opcode(new, op);

			// Add data.
			BYTE *data = (BYTE *)calloc(length + 4, sizeof(BYTE));
			if (data == NULL)
				return NULL;
			data[0] = bytes[pos+1];
			data[1] = bytes[pos+2];
			data[2] = bytes[pos+3];
			data[3] = bytes[pos+4];
			for (uint32_t j = 0; j < length; ++j)
				data[j+4] = bytes[pos + 5 + j];
			new->add_data(new, data, length + 4);

			// Shift pos, break the loop if pos at the end.
			pos = pos + 4 + length + 1;
			if (pos == size)
				break;
			else continue;
		}

		// Not PUSHDATA1 / PUSHDATA2 / PUSHDATA4.
		else
		{
			// Add opcode.
			opcode *op = (opcode *)calloc(1, sizeof(opcode));
			if (op == NULL)
				return NULL;
			*op = bytes[pos];
			new->add_opcode(new, op);

			// Shift pos, break the loop if pos at the end.
			pos = pos + 1;
			if (pos == size)
				break;
			else continue;
		}
	}

	// Check if PUSHDATA (1 to 75).
	else if (bytes[pos] >= 0x01 && bytes[pos] <= 0x4b)
	{
		// Check if enough bytes remain.
		uint32_t remain = size-pos-1;
		uint32_t length = bytes[pos];
		if (remain < length)
			return NULL;

		// Add PUSHDATA byte.
		opcode *op = (opcode *)calloc(1, sizeof(opcode));
		if (op == NULL)
			return NULL;
		*op = length;
		new->add_opcode(new, op);

		// Add data.
		BYTE *data = (BYTE *)calloc(length, sizeof(BYTE));
		if (data == NULL)
			return NULL;
		for (uint32_t j = 0; j < length; ++j)
			data[j] = bytes[pos + 1 + j];
		new->add_data(new, data, length);

		// Shift pos, break the loop if pos at the end.
		pos = pos + 0 + length + 1;
		if (pos == size)
			break;
		else continue;
	}

	// Unknow opcode.
	else
	{
		// Add OP_INVALIDOPCODE.
		opcode *op = (opcode *)calloc(1, sizeof(opcode));
		if (op == NULL)
			return NULL;
		*op = OP_INVALIDOPCODE;
		new->add_opcode(new, op);

		// Shift pos, break the loop if pos at the end.
		pos = pos + 1;
		if (pos == size)
			break;
		else continue;
	}
}
	return new;
}

void delete_Script(Script *self)
{
	delete_CLinkedlist(self->script);
	free(self);
}

bool is_opcode(BYTE byte)
{	
	if ( byte == 0x00 || (byte >= 0x4c && byte <= 0xb9) || byte == 0xff)
		return true;
	else return false;
}

const char * get_op_name(opcode op)
{
    switch (op)
    {
    // push value
    case OP_0                      : return "0";
    case OP_PUSHDATA1              : return "OP_PUSHDATA1";
    case OP_PUSHDATA2              : return "OP_PUSHDATA2";
    case OP_PUSHDATA4              : return "OP_PUSHDATA4";
    case OP_1NEGATE                : return "-1";
    case OP_RESERVED               : return "OP_RESERVED";
    case OP_1                      : return "1";
    case OP_2                      : return "2";
    case OP_3                      : return "3";
    case OP_4                      : return "4";
    case OP_5                      : return "5";
    case OP_6                      : return "6";
    case OP_7                      : return "7";
    case OP_8                      : return "8";
    case OP_9                      : return "9";
    case OP_10                     : return "10";
    case OP_11                     : return "11";
    case OP_12                     : return "12";
    case OP_13                     : return "13";
    case OP_14                     : return "14";
    case OP_15                     : return "15";
    case OP_16                     : return "16";

    // control
    case OP_NOP                    : return "OP_NOP";
    case OP_VER                    : return "OP_VER";
    case OP_IF                     : return "OP_IF";
    case OP_NOTIF                  : return "OP_NOTIF";
    case OP_VERIF                  : return "OP_VERIF";
    case OP_VERNOTIF               : return "OP_VERNOTIF";
    case OP_ELSE                   : return "OP_ELSE";
    case OP_ENDIF                  : return "OP_ENDIF";
    case OP_VERIFY                 : return "OP_VERIFY";
    case OP_RETURN                 : return "OP_RETURN";

    // stack ops
    case OP_TOALTSTACK             : return "OP_TOALTSTACK";
    case OP_FROMALTSTACK           : return "OP_FROMALTSTACK";
    case OP_2DROP                  : return "OP_2DROP";
    case OP_2DUP                   : return "OP_2DUP";
    case OP_3DUP                   : return "OP_3DUP";
    case OP_2OVER                  : return "OP_2OVER";
    case OP_2ROT                   : return "OP_2ROT";
    case OP_2SWAP                  : return "OP_2SWAP";
    case OP_IFDUP                  : return "OP_IFDUP";
    case OP_DEPTH                  : return "OP_DEPTH";
    case OP_DROP                   : return "OP_DROP";
    case OP_DUP                    : return "OP_DUP";
    case OP_NIP                    : return "OP_NIP";
    case OP_OVER                   : return "OP_OVER";
    case OP_PICK                   : return "OP_PICK";
    case OP_ROLL                   : return "OP_ROLL";
    case OP_ROT                    : return "OP_ROT";
    case OP_SWAP                   : return "OP_SWAP";
    case OP_TUCK                   : return "OP_TUCK";

    // splice ops
    case OP_CAT                    : return "OP_CAT";
    case OP_SUBSTR                 : return "OP_SUBSTR";
    case OP_LEFT                   : return "OP_LEFT";
    case OP_RIGHT                  : return "OP_RIGHT";
    case OP_SIZE                   : return "OP_SIZE";

    // bit logic
    case OP_INVERT                 : return "OP_INVERT";
    case OP_AND                    : return "OP_AND";
    case OP_OR                     : return "OP_OR";
    case OP_XOR                    : return "OP_XOR";
    case OP_EQUAL                  : return "OP_EQUAL";
    case OP_EQUALVERIFY            : return "OP_EQUALVERIFY";
    case OP_RESERVED1              : return "OP_RESERVED1";
    case OP_RESERVED2              : return "OP_RESERVED2";

    // numeric
    case OP_1ADD                   : return "OP_1ADD";
    case OP_1SUB                   : return "OP_1SUB";
    case OP_2MUL                   : return "OP_2MUL";
    case OP_2DIV                   : return "OP_2DIV";
    case OP_NEGATE                 : return "OP_NEGATE";
    case OP_ABS                    : return "OP_ABS";
    case OP_NOT                    : return "OP_NOT";
    case OP_0NOTEQUAL              : return "OP_0NOTEQUAL";
    case OP_ADD                    : return "OP_ADD";
    case OP_SUB                    : return "OP_SUB";
    case OP_MUL                    : return "OP_MUL";
    case OP_DIV                    : return "OP_DIV";
    case OP_MOD                    : return "OP_MOD";
    case OP_LSHIFT                 : return "OP_LSHIFT";
    case OP_RSHIFT                 : return "OP_RSHIFT";
    case OP_BOOLAND                : return "OP_BOOLAND";
    case OP_BOOLOR                 : return "OP_BOOLOR";
    case OP_NUMEQUAL               : return "OP_NUMEQUAL";
    case OP_NUMEQUALVERIFY         : return "OP_NUMEQUALVERIFY";
    case OP_NUMNOTEQUAL            : return "OP_NUMNOTEQUAL";
    case OP_LESSTHAN               : return "OP_LESSTHAN";
    case OP_GREATERTHAN            : return "OP_GREATERTHAN";
    case OP_LESSTHANOREQUAL        : return "OP_LESSTHANOREQUAL";
    case OP_GREATERTHANOREQUAL     : return "OP_GREATERTHANOREQUAL";
    case OP_MIN                    : return "OP_MIN";
    case OP_MAX                    : return "OP_MAX";
    case OP_WITHIN                 : return "OP_WITHIN";

    // crypto
    case OP_RIPEMD160              : return "OP_RIPEMD160";
    case OP_SHA1                   : return "OP_SHA1";
    case OP_SHA256                 : return "OP_SHA256";
    case OP_HASH160                : return "OP_HASH160";
    case OP_HASH256                : return "OP_HASH256";
    case OP_CODESEPARATOR          : return "OP_CODESEPARATOR";
    case OP_CHECKSIG               : return "OP_CHECKSIG";
    case OP_CHECKSIGVERIFY         : return "OP_CHECKSIGVERIFY";
    case OP_CHECKMULTISIG          : return "OP_CHECKMULTISIG";
    case OP_CHECKMULTISIGVERIFY    : return "OP_CHECKMULTISIGVERIFY";

    // expansion
    case OP_NOP1                   : return "OP_NOP1";
    case OP_CHECKLOCKTIMEVERIFY    : return "OP_CHECKLOCKTIMEVERIFY";
    case OP_CHECKSEQUENCEVERIFY    : return "OP_CHECKSEQUENCEVERIFY";
    case OP_NOP4                   : return "OP_NOP4";
    case OP_NOP5                   : return "OP_NOP5";
    case OP_NOP6                   : return "OP_NOP6";
    case OP_NOP7                   : return "OP_NOP7";
    case OP_NOP8                   : return "OP_NOP8";
    case OP_NOP9                   : return "OP_NOP9";
    case OP_NOP10                  : return "OP_NOP10";

    case OP_INVALIDOPCODE          : return "OP_INVALIDOPCODE";

    default:
        return "OP_UNKNOWN";
    }
}

bool Script_add_opcode(Script *self, opcode *op)
{
	if ( (self->script->add(self->script, op, 1)) )
		return true;
	else return false;
}

bool Script_add_data(Script *self, BYTE *data, size_t size)
{
	if ( (self->script->add(self->script, data, size)) )
		return true;
	else return false;
}

uint8_t * Script_to_string(Script *self, size_t *size)
{
	CLinkedlistNode **statements = self->script->forward_traversing(self->script);
	CLinkedlist *statements_str = new_CLinkedlist();

// Script statements loop, convert one statement to one string and add to linked list.
for (uint32_t i = 0; i < self->script->length; ++i)
{
	switch (statements[i]->size)
	{	// If opcode.
		case 1:
		{
			opcode op_buffer = ((opcode*)(statements[i]->data))[0];

			// Opcode is PUSHDATA (1 ~ 75): "PUSHDATA(size)"
			if (op_buffer >= 0x01 && op_buffer <= 0x4b)
			{
				int8_t size_str[3];
				BYTE size_byte[1];
				size_byte[0] = op_buffer;
				bytearr_to_hexstr(size_byte, 1, size_str);

				uint8_t *str = (uint8_t *)calloc(12, sizeof(uint8_t));
				str[0] = 'P'; str[1] = 'U'; str[2] = 'S'; str[3] = 'H';
				str[4] = 'D'; str[5] = 'A'; str[6] = 'T'; str[7] = 'A';
				str[8] = '(';
				str[9] = size_str[0]; str[10] = size_str[1];
				str[11] = ')';

				statements_str->add(statements_str, str, 12 * sizeof(uint8_t));
				break;
			}

			// Opcode is PUSHDATA1/2/4: "OP_PUSHDATA1" "OP_PUSHDATA2" "OP_PUSHDATA4"
			else if (op_buffer == OP_PUSHDATA1 || op_buffer == OP_PUSHDATA2 || op_buffer == OP_PUSHDATA4)
			{
				const char * op_name = get_op_name(op_buffer);
				size_t len = strlen(op_name);
				uint8_t *str = (uint8_t *)calloc(len, sizeof(uint8_t));
				for (uint32_t i = 0; i < len; ++i)
					str[i] = op_name[i];
				statements_str->add(statements_str, str, len * sizeof(uint8_t));
				break;
			}

			// Opcode is not PUSHDATA(1 ~ 75)/1/2/4: "OP_XXXXX "
			else
			{
				const char * op_name = get_op_name(op_buffer);
				size_t len = strlen(op_name) + 1;
				uint8_t *str = (uint8_t *)calloc(len, sizeof(uint8_t));
				for (uint32_t i = 0; i < len; ++i)
					str[i] = op_name[i];
				str[len - 1] = ' ';
				statements_str->add(statements_str, str, len * sizeof(uint8_t));
				break;
			}
		}

		// If data bytes: "[bytes] "
		default:
		{
			uint32_t data_byte_len = statements[i]->size;
			uint32_t data_str_len  = data_byte_len * 2;
			uint8_t data_str[data_str_len + 1];

			bytearr_to_hexstr((BYTE *)(statements[i]->data), data_byte_len, (int8_t *)data_str);
			uint8_t *str = (uint8_t *)calloc(data_str_len + 3, sizeof(uint8_t));

			str[0] = '[';
			for (uint32_t i = 0; i < data_str_len; ++i)
				str[i+1] = data_str[i];
			str[data_str_len + 1] = ']';
			str[data_str_len + 2] = ' ';

			statements_str->add(statements_str, str, (data_str_len + 3) * sizeof(uint8_t));
			break;
		}
	}
}
free(statements);

	// Linked list to a single string.
	*size = statements_str->total_size(statements_str);
	uint8_t *string = (uint8_t *)malloc(statements_str->total_size(statements_str));
	CLinkedlistNode **list = statements_str->forward_traversing(statements_str);

	uint32_t pos = 0;
	for (uint32_t i = 0; i < statements_str->length; ++i)
	{
		for (uint32_t j = 0; j < (list[i]->size)/sizeof(uint8_t); ++j)
		{
			string[pos] = ((uint8_t *)(list[i]->data))[j];
			pos++;
		}
	}
	free(list);
	delete_CLinkedlist(statements_str);
	return string;
}

BYTE * Script_to_bytes(Script *self, size_t *size)
{
	uint32_t script_len = self->script->length;
	size_t   total_size = self->script->total_size(self->script);
	*size = total_size;

	BYTE *bytes = (BYTE *)malloc(total_size);
	CLinkedlistNode **list = self->script->forward_traversing(self->script);

	uint32_t pos = 0;
	// Script statements loop.
	for (size_t i = 0; i < script_len; ++i)
	{
		// Statements bytes loop.
		for (size_t j = 0; j < list[i]->size; ++j)
			bytes[pos+j] = ((BYTE *)(list[i]->data))[j];

		// Shift the posistion.
		pos = pos + list[i]->size;
	}
	free(list);
	return bytes;
}