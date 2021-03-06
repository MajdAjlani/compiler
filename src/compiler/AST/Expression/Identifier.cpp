#include "Identifier.h"
#include "../../Symbol Table/symbolTable.h"
#include "../../Symbol Table/Method.h"
#include "AutoConst.h"
#include "../Object/Procedure.h"

bool Identifier::leftAssignment = false;

bool Identifier::isAssigned = true;

bool Identifier::isStaticMethod = true;

Identifier::Identifier(Node* preDot, Symbol* postDot, bool isArray) : Expression(Node::current)
{
	this->preDot = preDot;
	this->postDot = postDot;
	this->isArray = isArray;
	this->isConst = this->isReadonly = this->isType = false;
}

string Identifier::getType()
{
	return "identifier";
}

Symbol* Identifier::getPostDot() {
	return this->postDot;
}

int Identifier::print(int nodeCnt)
{

	fprintf(nodesFile, "{ id:%d, label:'%s', shape: 'box', color:'#b1e2de'},", nodeCnt, this->postDot->getName().c_str());

	return nodeCnt;
}

Node* Identifier::getPreDot() {
	return preDot;
}

bool Identifier::getIsConst() {
	return this->isConst;
}

bool Identifier::getIsReadonly() {
	return this->isReadonly;
}

void Identifier::setArrayDimensions(queue<Node*>dimensions) {
	
	this->isArray = true;

	while (!dimensions.empty())
		this->dimensions.push_back(dimensions.front()), dimensions.pop();
	return;
}

string Identifier::getPath() {
	if (preDot != nullptr)
		return ((Identifier*)preDot)->getPath() + "." + this->postDot->getName();
	return this->postDot->getName();
}

bool Identifier::typeChecking() {

	Symbol* prev = nullptr;

	if (this->preDot != nullptr) {
		this->preDot->typeChecking();
		pair<TypeExpression*, Symbol*>res = TypesTable::getType(this->preDot->nodeType->typeExpression());
		if (res.first->getTypeId() == TYPE_ERROR) {
			this->nodeType = res.first;
			return true;
		}
		if (res.second == nullptr) {
			this->nodeType = new TypeError("invalid Dot Operator on primitive type", postDot->getLineNo());
			return true;
		}
		if (res.first->getTypeId() == TYPE_ARRAY) {
			if (this->postDot->getName() == "Length") {
				this->nodeType = TypesTable::getType("INT").first;
				this->isConst = true;
			}
			else {
				this->nodeType = new TypeError("Array Type doesn't contain '" + postDot->getName() + "' identifier.", postDot->getLineNo());
			}
			return true;
		}
		prev = res.second;
	}

	prev = symbolTable::findIdentifier(postDot, (symbolTable*)this->symboltable, prev);

	postDot->offset = prev->offset;

	if (postDot->getName() == "this" || postDot->getName() == "base") {
		if (Identifier::isStaticMethod)
			this->nodeType = new TypeError("use '" + postDot->getName() + "' keyword in static method is not allowed", postDot->getLineNo());
		else
			this->nodeType = TypesTable::findOrCreate(((Class*)prev)->getFullPath(), prev);
		string lastName = postDot->getName();
		*postDot = *prev;
		postDot->setName(lastName);
		return true;
	}

	if (Identifier::isStaticMethod && (preDot == nullptr && prev->getType() == "field" && !((Field*)prev)->getIsStatic())) {
		this->nodeType = new TypeError("cannot use non static field '" + postDot->getName() +"' on static method", postDot->getLineNo());
		return true;
	}

	//we didnt find identifier
	if (prev->getColNo() == -15) {
		//try to find it as static field in class !!
		if (preDot == nullptr ||  preDot->getType() == "identifier" && ((Identifier*)preDot)->isType) {
			
			this->isType = true;
			
			//up to parent class
			symbolTable* parentRef = (symbolTable*)this->symboltable;
			while (parentRef != nullptr) {
				if (parentRef->get_owner() != nullptr && parentRef->get_owner()->getType() == "class")
					break;
				parentRef = parentRef->get_parent();
			}

			string requiredName = getPath();
			
			prev = symbolTable::findType(((Class*)parentRef->get_owner())->get_type_graph_position(), requiredName);
				
			if (prev != nullptr) {
				this->nodeType = TypesTable::findOrCreate(prev->getFullPath(), prev);
				postDot = prev;
				return true;
			}
		}
		this->nodeType = new TypeError("undeclared identifier " + postDot->getName(), postDot->getLineNo());
		return true;
	}

	if (prev->isComplex()) {
		this->nodeType = TypesTable::findOrCreate(((Class*)prev->getTypeRef())->getFullPath(), prev->getTypeRef());
	}
	else {
		if (prev->getType() == "field") {
			this->nodeType = TypesTable::getType(((Field*)prev)->get_type_name()).first;
		}
		else if (prev->getType() == "localvariable") {
			this->nodeType = TypesTable::getType(((LocalVariable*)prev)->get_type_name()).first;
		}
	}

	//this if for check if this Identifier is const or not
	if (prev->getType() == "field") {
		if (((Field*)prev)->getIsConst())
			this->isConst = true;
		if (((Field*)prev)->getIsReadonly())
			this->isReadonly = true;
		//throw error when access to static field from this keyword
		if (((Field*)prev)->getIsStatic()) {
			if (preDot != nullptr && preDot->getType() == "identifier" && (((Identifier*)preDot)->postDot->getName() == "this" || ((Identifier*)preDot)->postDot->getName() == "base")) {
				this->nodeType = new TypeError("cannot access to static field from 'this' keyword or objects", postDot->getLineNo());
				return true;
			}
		}
		else if (preDot && preDot->getType() == "identifier" && ((Identifier*)preDot)->isType) {
			this->nodeType = new TypeError("cannot access to no static field from it's type name", postDot->getLineNo());
			return true;
		}

		//handle arrays
		if (((Field*)prev)->getDimension() != this->dimensions.size()) {
			if (((Field*)prev)->getDimension() == 0) {
				this->nodeType = new TypeError("cannot apply indexing to un array type", prev->getLineNo());
				return true;
			}
			if (this->dimensions.size() != 0) {
				this->nodeType = new TypeError("wrong number of indices, expected " + to_string(((Field*)prev)->getDimension()), prev->getLineNo());
				return true;
			}
			this->nodeType = new TypeArray(this->nodeType, this->dimensions.size());
		}
	}
	else if (prev->getType() == "localvariable") {
		if (((LocalVariable*)prev)->getIsConst())
			this->isConst = true;
		//handle arrays
		if (((LocalVariable*)prev)->getDimension() != this->dimensions.size()) {
			if (((LocalVariable*)prev)->getDimension() == 0) {
				this->nodeType = new TypeError("cannot apply indexing to un array type", prev->getLineNo());
				return true;
			}
			if (this->dimensions.size() != 0) {
				this->nodeType = new TypeError("wrong number of indices, expected " + to_string(((LocalVariable*)prev)->getDimension()), prev->getLineNo());
				return true;
			}
			this->nodeType = new TypeArray(this->nodeType, ((LocalVariable*)prev)->getDimension());
		}
	}

	postDot = prev;
	return true;
	
	/*
	//this for check un assigned variable
	if (preDot == nullptr) {
		// using unassigned variable
		if (prev->getType() == "localvariable" && !((LocalVariable*)prev)->isInitialized()) {
			if (!Identifier::leftAssignment) {
				new TypeError("Warning for using unassigned variable", divs[0]->getLineNo());
			}
			else {
				if (divs.size() > 1) {
					new TypeError("Warning for using Dot operator in unassigned variable", divs[i]->getLineNo());
				}
				Identifier::isAssigned = false;
			}
		}
	}*/
}

void Identifier::generateCode() {
	// static fields
	if (postDot->getType() == "field" && ((Field*)postDot)->getIsStatic()) {
		AsmGenerator::addInstruction("lw $t0, " + postDot->getFullPath());
		AsmGenerator::push("t0");
		return;
	}

	if (preDot != nullptr) {
		preDot->generateCode();
		AsmGenerator::pop("t0");
		AsmGenerator::lw("t0", "t0", -1 * postDot->offset);
	}
	else {
		if (postDot->getType() != "field") {
			if (postDot->getName() == "this") {
				AsmGenerator::lw("t0", "fp", -4);
			}
			else {
				AsmGenerator::lw("t0", "fp", -1 * postDot->offset);
			}
		}
		else {
			AsmGenerator::lw("t0", "fp", -4);
			AsmGenerator::lw("t0", "t0", -1 * postDot->offset);
		}
		if (dimensions.size() > 0) {
			AsmGenerator::push("t0");
			// handle only one dimension right now!
			dimensions[0]->generateCode();
			AsmGenerator::pop("t1");
			AsmGenerator::addInstruction("addi $t0, $0, -4");
			AsmGenerator::addInstruction("mul $t1, $t0, $t1");
			
			AsmGenerator::pop("t0");

			AsmGenerator::addInstruction("sub $t0, $t0, $t1");
			AsmGenerator::lw("t0", "t0", 0);
		}
	}
	AsmGenerator::push("t0");
}

Identifier::~Identifier()
{
}
