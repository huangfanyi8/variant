#include <iostream>
#include <type_traits>
#include"variant.hpp"
#include<string>


using std::cout;

int main()
{
  variant<std::string,const char*>  v1{"hello world"};

  std::cout<<v1.index()<<'\n';

  std::cout<<v1.get<1>()<<'\n';

  variant<std::string,const char*>  v2="hh";

  v2=v1;
  std::cout<<v2.index()<<'\n';

  std::cout<<v2.get<1>()<<'\n';
  std::cout<<v1.index()<<'\n';

  std::cout<<v1.get<1>()<<'\n';

}