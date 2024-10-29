#include <array>
#include <execution>
#include <numeric>


const std::array<double, 4>a = {{0., 1., 2., 3.}};

int main()
{
	double res = std::reduce(std::execution::par,
	             a.begin(), a.end(), 0.0, std::plus<>());
	return 0;
}
