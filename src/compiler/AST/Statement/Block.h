#pragma once
#include "Statement.h"
class Block :public Statement
{
	vector<Statement*> statements;
public:
	Block();
	void add(Statement* statement);
	~Block();
};
