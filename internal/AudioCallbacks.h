namespace LiveKit
{
	typedef bool(*AudioWriteCallback)(short* buf, size_t buf_size, void* user_ptr);
	typedef bool(*AudioReadCallback)(const short* buf, size_t buf_size, void* user_ptr);
	typedef void(*EOFCallback)(void* user_ptr);
}

