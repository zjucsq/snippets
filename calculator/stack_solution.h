#pragma once
#include <stack>
#include <string>
#include <unordered_map>

class Solution {
public:
    Solution() {
        priority = { {'(', 0},{')', 0},{'+', 1},{'-', 1},{'*', 2},{'/', 2} };
    }

    int calculate(std::string s) {
        s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
        std::stack<int> s_op, s_num;

        int pos = 0;
        while (pos < s.size()) {
            char c = s[pos];
            if (c >= '0' && c <= '9') {
                int tmp = 0;
                while (s[pos] >= '0' && s[pos] <= '9') {
                    tmp *= 10;
                    tmp += s[pos] - '0';
                    pos++;
                }
                s_num.push(tmp);
                --pos;
            }
            else if (c == '(') {
                s_op.push(c);
            }
            else if (c == ')') {
                while (s_op.top() != '(') {
                    int num2 = s_num.top();
                    s_num.pop();
                    int num1 = s_num.top();
                    s_num.pop();
                    char op = s_op.top();
                    s_op.pop();
                    s_num.push(calc_op(num1, num2, op));
                }
                s_op.pop();
            }
            else {
                while (!s_op.empty() && priority[c] <= priority[s_op.top()]) {
                    int num2 = s_num.top();
                    s_num.pop();
                    int num1 = s_num.top();
                    s_num.pop();
                    char op = s_op.top();
                    s_op.pop();
                    s_num.push(calc_op(num1, num2, op));
                }
                s_op.push(c);
            }
            ++pos;
        }
        while (!s_op.empty()) {
            int num2 = s_num.top();
            s_num.pop();
            int num1 = s_num.top();
            s_num.pop();
            char op = s_op.top();
            s_op.pop();
            s_num.push(calc_op(num1, num2, op));
        }

        return s_num.top();
    }
private:
    std::unordered_map<char, int> priority;

    int calc_op(int num1, int num2, char op) {
        int res = 0;
        if (op == '+') {
            res = num1 + num2;
        }
        else if (op == '-') {
            res = num1 - num2;
        }
        else if (op == '*') {
            res = num1 * num2;
        }
        else if (op == '/') {
            res = num1 / num2;
        }
        return res;
    }
};