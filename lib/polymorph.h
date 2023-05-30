#ifndef POLYMORPH_H
#define POLYMORPH_H

#include <variant>

template <typename... Ts>
class Polymorph : public std::variant<Ts...>
{
public:
  using std::variant<Ts...>::variant;

  template <typename T>
  bool is() const
  {
    return std::holds_alternative<T>(*this);
  }

  template <typename T>
  T get() const
  {
    return std::get<T>(*this);
  }

private:

};

#endif // POLYMORPH_H
