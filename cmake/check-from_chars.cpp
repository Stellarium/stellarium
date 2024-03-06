#include <charconv>

int main()
{
	const char str[] = "1.2345e3";
	float num;
	std::from_chars(str, str + sizeof str - 1, num);
}
