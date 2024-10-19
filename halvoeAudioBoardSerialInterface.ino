// Visual Micro is in vMicro>General>Tutorial Mode
// 
/*
    Name:       halvoeAudioBoardSerialInterface.ino
    Created:	19.10.2024 10:11:46
    Author:     LAPTOP-8TKMEDGB\samue
*/

#include "SerialInterface.hpp"

using namespace halvoe;

static constexpr const size_t c_serializerBufferSize = 1024;
static constexpr const size_t c_deserializerBufferSize = 1024;

class SerialImpl : public SerialInterface<c_serializerBufferSize, c_deserializerBufferSize>
{
	public:
		SerialImpl(HardwareSerial& in_serial) : SerialInterface<c_serializerBufferSize, c_deserializerBufferSize>(in_serial)
		{}
	
		bool setup()
		{
			return true;
		}

	private:
		bool doHandleData(Deserializer<c_deserializerBufferSize>&& in_deserializer, SerialDataCode in_code)
		{
			return false;
		}

		bool doHandleCommand(Deserializer<c_deserializerBufferSize>&& in_deserializer, SerialCommandCode in_code)
		{
			switch (in_code)
			{
				case SerialCommandCode::playFile:
					SerialStringSizeType filenameSize = 0;
					auto filenamePointer = in_deserializer.read<SerialStringSizeType>(64, filenameSize);
					String filename(filenamePointer.get(), filenameSize);
					return true;
			}

			return false;
		}
};

SerialImpl serialImpl(Serial1);
String string;

// The setup() function runs once each time the micro-controller starts
void setup()
{
	serialImpl.setup();
	string = "Hello World!";
}

// Add the main program code into the continuous loop() function
void loop()
{
	serialImpl.receiveMessage();
	auto message = serialImpl.beginMessage(SerialMessageType::command);
	message.m_serializer.write(string.c_str(), string.length());
	serialImpl.sendMessage(std::move(message));
}
