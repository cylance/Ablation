/*
* Original Sample from http://www.cplusplus.com/doc/tutorial/polymorphism/
*/
#include <iostream>

using namespace std;

class Polygon {
protected:
	int width, height;
public:
	Polygon(int a, int b) : width(a), height(b) {}
	virtual int area(void) = 0;
	virtual int ret2(void) = 0;
	virtual int ret3(void) = 0;
	virtual int ret4(void) = 0;
	virtual int ret5(void) { return 5; }
	virtual int ret6(void) { return std::getchar(); }

	void printarea()
	{
		cout << this->area() + this->ret4() + this->ret5() + this->ret6() << '\n';
	}
};

class Rectangle : public Polygon {
public:
	Rectangle(int a, int b) : Polygon(a, b) {}
	int area()
	{
		return width*height + this->ret2();
	}

	int ret2() { return 2; }
	int ret3() { return 3; }
	int ret4() { return 4; }
	int ret6() { return 6; }
};

class Triangle : public Polygon {
public:
	Triangle(int a, int b) : Polygon(a, b) {}
	int area()
	{
		return width*height / 2 + this->ret3();
	}

	int ret2() { return 2 * 2; }
	int ret3() { return 3 * 2; }
	int ret4() { return 4 * 2; }
	int ret6() { return 6 * 2; }

};

bool test = false;

int main()
{
	Polygon * ppoly1 = new Rectangle(4, 5);
	Polygon * ppoly2 = new Triangle(4, 5);
	
	ppoly1->printarea();
	
	if (test)
		ppoly2->printarea();

	if (!test)
		ppoly2->printarea();


	delete ppoly1;
	delete ppoly2;

	return 0;
}
