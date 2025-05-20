
/// <summary>
/// Convert positive number to a string.
/// </summary>
/// <param name="number">number to convert</param>
/// <param name="output">pointer to a char* to drop the result too (should LocalFree) when done</param>
/// <param name="output_size">is the size of the buffer output will be</param>
/// <returns>true if ok and false on error. Note output and outputsize will be set to 0 on error also if not null</returns>
extern "C" {
	extern bool NumberToString(int number, char** output, int* output_size);
	extern bool StringToNumber(const char* input, int* output);
}