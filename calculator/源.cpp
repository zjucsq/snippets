#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <variant>
#include <optional>
#include <algorithm>
#include <variant>
#include <cctype>

// ������ȷ�����ȼ����ķ�
// exp �� exp addop term | term
// exp �� term { addop term } # EBNF version
// addop �� + | -
// term �� term mulop factor | factor
// term �� factor { mulop factor } # EBNF version
// mulop �� *
// factor �� ( exp ) | number

// https://zhuanlan.zhihu.com/p/24615333
// expr := term (+|-) term (+|-) ... (+|-) term
// term := factor (*|/) factor (* | /) ... (*|/) factor
// factor := INT | function | "(" expr ")" | (+|-)factor
// function := fact "(" expr ")" | pow "(" expr "," expr ")"

class Input_t {
public:
    std::string_view str;
    size_t idx;

    Input_t(std::string& s, size_t index = 0) : str(s), idx(index) {}
    Input_t(std::string_view s, size_t index = 0) : str(s), idx(index) {}

    char get_char() const {
        if (idx >= str.length()) {
            return '\0'; // ʹ�� '\0' ��ʾ�ַ����Ľ���
        }
        return str[idx];
    }

    Input_t next() const {
        return Input_t(str, idx + 1);
    }
};

// ����һ���ṹ����������������µ�����״̬
template<typename T>
struct ParserResult {
    std::optional<T> result;
    Input_t next_input;

    ParserResult(std::optional<T> res, Input_t next) : result(std::move(res)), next_input(std::move(next)) {}
};

// ����һ���������������ͣ�������һ�����벢����һ���������
template<typename T>
using ParserFunction = std::function<ParserResult<T>(Input_t)>;

template <typename L1, typename L2> struct Concat {};

template <template <typename...> typename TypeList, typename... Ts1,
    typename... Ts2>
struct Concat<TypeList<Ts1...>, TypeList<Ts2...>> {
    using type = TypeList<Ts1..., Ts2...>;
};

template<typename ...T>
struct ParserFunctionTypes {};

template<typename T>
struct ParserFunctionTypes<ParserFunction<T>> {
    using type = std::tuple<T>;
};

template<typename T, typename ... ParserFunctions>
struct ParserFunctionTypes<ParserFunction<T>, ParserFunctions...> {
    using type = Concat<std::tuple<T>, typename ParserFunctionTypes<ParserFunctions...>::type>::type;
};

// ������������ַ��ĺ���
ParserResult<char> char_parser(Input_t input, char c) {
    if (input.get_char() == c) {
        return { c, input.next() };
    }
    return { std::nullopt, input };
}

auto get_char_parser = [](char c) -> ParserFunction<char> {
    return [c](Input_t input) { return char_parser(input, c); };
};

// ��������������ϵĺ��������� p_and �� p_or
template<typename T1, typename T2>
ParserFunction<std::pair<T1, T2>> p_and(const ParserFunction<T1>& parser1, const ParserFunction<T2>& parser2) {
    return [parser1, parser2](Input_t input) -> ParserResult<std::pair<T1, T2>> {
        auto res1 = parser1(input);
        if (!res1.result.has_value()) {
            return { std::nullopt, input };
        }
        auto res2 = parser2(res1.next_input);
        if (!res2.result.has_value()) {
            return { std::nullopt, input };
        }
        return { std::make_pair(* res1.result, *res2.result), res2.next_input };
    };
}

ParserFunction<std::tuple<>> p_and_combine() {
    return [](Input_t input) -> ParserResult<std::tuple<>> {
        return { std::tuple<>{}, input };
    };
}

template<typename T, typename... Parsers>
ParserFunction<typename Concat<std::tuple<T>, typename ParserFunctionTypes<Parsers...>::type>::type> p_and_combine(const ParserFunction<T>& first, Parsers... rest) {
    return [first, rest...](Input_t input) -> ParserResult<typename Concat<std::tuple<T>, typename ParserFunctionTypes<Parsers...>::type>::type> {
        auto res1 = first(input);
        if (!res1.result.has_value()) {
            return { std::nullopt, input };
        }
        auto res2 = p_and_combine(rest...)(res1.next_input);
        if (!res2.result.has_value()) {
            return { std::nullopt, input };
        }
        return { std::tuple_cat(std::make_tuple(*res1.result), *res2.result), res2.next_input};
    };
}

// ���� p_or ����
template<typename T>
ParserFunction<T> p_or(const ParserFunction<T>& parser1, const ParserFunction<T>& parser2) {
    return [parser1, parser2](Input_t input) -> ParserResult<T> {
        auto res1 = parser1(input);
        if (res1.result.has_value()) {
            return res1;
        }
        return parser2(input);
    };
}

template<typename T>
ParserFunction<T> p_or_combine(const ParserFunction<T>& parser) {
    return parser;
}

template<typename T, typename... Parsers>
ParserFunction<T> p_or_combine(const ParserFunction<T>& first, Parsers... rest) {
    return [first, rest...](Input_t input) -> ParserResult<T> {
        auto result = first(input);
        if (result.result.has_value()) {
            return result;
        }
        return p_or_combine(rest...)(input);
    };
}

// ���� p_many ����
template<typename T>
ParserFunction<std::vector<T>> p_many(const ParserFunction<T>& parser) {
    return [parser](Input_t input) -> ParserResult<std::vector<T>> {
        std::vector<T> values;
        while (true) {
            auto res = parser(input);
            if (!res.result.has_value()) {
                break;
            }
            values.push_back(*res.result);
            input = res.next_input;
        }
        return { values, input };
    };
}

template<typename T>
ParserFunction<std::vector<T>> p_many1(const ParserFunction<T>& parser) {
    return p_and(parser, p_many(parser));
}

ParserFunction<char> add_parser = [](Input_t input) { return char_parser(input, '+'); };
ParserFunction<char> sub_parser = [](Input_t input) { return char_parser(input, '-'); };
ParserFunction<char> mul_parser = [](Input_t input) { return char_parser(input, '*'); };
ParserFunction<char> div_parser = [](Input_t input) { return char_parser(input, '/'); };
ParserFunction<char> lbr_parser = [](Input_t input) { return char_parser(input, '('); };
ParserFunction<char> rbr_parser = [](Input_t input) { return char_parser(input, ')'); };

ParserFunction<char> addsub_parser = p_or_combine(add_parser, sub_parser);
ParserFunction<char> muldiv_parser = p_or_combine(mul_parser, div_parser);

ParserFunction<int> int_parser = [](Input_t input) -> ParserResult<int> {
    if (std::isdigit(input.get_char())) {
        int res = 0;
        while (std::isdigit(input.get_char())) {
            res = res * 10 + input.get_char() - '0';
            input = input.next();
        }
        return { res, input };
    }
    return { std::nullopt, input };
};

ParserFunction<int> signint_parser = [](Input_t input) -> ParserResult<int> {
    auto [r, i] = p_and(addsub_parser, int_parser)(input);
    if (r.has_value()) {
        int res = r.value().second;
        if (r.value().first == '-') {
            res = -res;
        }
        return { res, input };
    }
    return { std::nullopt, input };
};

ParserFunction<int> expr_parser, term_parser, factor_parser;

ParserFunction<int> fact_parser = [](Input_t input) -> ParserResult<int> {
    auto [r, input_1] = p_and_combine(get_char_parser('f'), get_char_parser('a'), get_char_parser('c'), get_char_parser('t'), get_char_parser('('), expr_parser, get_char_parser(')'))(input);
    if (r.has_value()) {
        auto tmp = r.value();
        int res = std::get<5>(tmp);
        if (res == 0) {
            res = 1;
        }
        else {
            for (int i = 2; i < res; ++i) {
                res *= 2;
            }
        }
        return { res, input_1};
    }
    return { std::nullopt, input };
};

ParserFunction<int> term_br_parser = [](Input_t input) -> ParserResult<int> {
    auto [r, input_1] = p_and(p_and(lbr_parser, expr_parser), rbr_parser)(input);
    if (r.has_value()) {
        auto tmp = r.value();
        int res = tmp.first.second;
        return { res, input_1 };
    }
    return { std::nullopt, input };
};

int calculate(std::string s) {
    s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
    Input_t input(s);
    auto [r, i] = expr_parser(input);
    return r.value();
}

int main() {
    // Define expr_parser, term_parser, factor_parser here to avoid circular dependency
    expr_parser = [](Input_t input) -> ParserResult<int> {
        auto [r, input_1] = p_and(term_parser, p_many(
            p_and(addsub_parser, term_parser)))(input);
        auto tmp = r.value();
        int res = tmp.first;
        std::vector<std::pair<char, int>> lst = tmp.second;
        for (auto [op, val] : lst) {
            if (op == '-') {
                res -= val;
            }
            else {
                res += val;
            }
        }
        return { res, input_1 };
    };

    term_parser = [](Input_t input) -> ParserResult<int> {
        auto [r, input_1] = p_and(factor_parser, p_many(
            p_and(muldiv_parser, factor_parser)))(input);
        auto tmp = r.value();
        int res = tmp.first;
        std::vector<std::pair<char, int>> lst = tmp.second;
        for (auto [op, val] : lst) {
            if (op == '*') {
                res *= val;
            }
            else {
                res /= val;
            }
        }
        return { res, input_1 };
    };

    factor_parser = p_or_combine(int_parser, signint_parser, term_br_parser);
    // factor_parser = p_or(p_or(int_parser, signint_parser), term_br_parser);

    // Use the calculate function to get the result of an arithmetic expression
    std::string expression = "3 + 5 * (10 - fact(3))";
    int result = calculate(expression);
    std::cout << "Result: " << result << std::endl;

    return 0;
}