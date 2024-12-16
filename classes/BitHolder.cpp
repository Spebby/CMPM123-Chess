#include "BitHolder.h"
#include "Bit.h"

#ifdef DEBUG
#include "../tools/Logger.h"
#endif

BitHolder::~BitHolder() {

}

// const version checks for bit without picking it up from the holder
Bit* BitHolder::bit() const {
	return _bit;
}

Bit* BitHolder::bit() {
	if (_bit && _bit->getParent() != this && !_bit->isPickedUp()) {
		_bit = nullptr;
	}
	return _bit;
}

// TODO: eventually I'll have to go back and rework this framework code since me messing with it has caused
// things to break. Eventually revert it back to Graeme's original version or slowly move it back towards that.
void BitHolder::setBit(Bit* abit) {
	if (abit != (void *)bit()) {
		destroyBit();
		_bit = abit;
		if (_bit) {
			_bit->setParent(this);
			_bit->setPosition(this->getPosition());
			int tag = abit->gameTag();
			#ifdef DEBUG
			//Loggy.log(std::to_string((tag & 8) >> 3) + " placed " + std::to_string(tag & 7) + " at (" + std::to_string(abit->getPosition().x) + ", " + std::to_string(abit->getPosition().y) + ")");
			#endif
		}
	}
}

void BitHolder::destroyBit() {
	if (_bit) {
		delete _bit;
		_bit = nullptr;
	}
}

Bit* BitHolder::canDragBit(Bit* bit, Player* player) {
	if (bit->getParent() == this && bit->friendly()) {
		return bit;
	}
	return nullptr;
}

void BitHolder::cancelDragBit(Bit* bit) {
	setBit(bit);
}

void BitHolder::draggedBitTo(Bit* bit, BitHolder* dst) {
	setBit(nullptr);
}

bool BitHolder::canDropBitAtPoint(Bit* bit, const ImVec2& point) {
	return true;
}

void BitHolder::willNotDropBit(Bit *bit) {

}

bool BitHolder::dropBitAtPoint(Bit *bit, const ImVec2 &point) {
	setBit (bit);
	return true;
}

void BitHolder::initHolder(const ImVec2 &position, const ImVec4 &color, const char *spriteName) {
	setPosition(position.x, position.y);
	setColor(color.x, color.y, color.z, color.w);
	setSize(0, 0);
	setScale(1.0f);
	setLocalZOrder(0);
	setHighlighted(false);
	setGameTag(0);
	setBit(nullptr);
	LoadTextureFromFile(spriteName);
}