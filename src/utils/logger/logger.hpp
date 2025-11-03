#include <iostream>

class Logger
{
public:
	bool isEnabled;

	template<class T>
	Logger& operator<<(const T& value)
	{
		if (isEnabled)
			std::cout << value;
		return *this;
	}	

	Logger& operator<< (std::ostream& (*manip)(std::ostream&))
	{
		if (isEnabled)
		std::cout << manip;
		return *this;
	}
};