#pragma once

#include <BasicSerializer.hpp>
#include <Arduino.h>
#include "halvoeLog.hpp"

namespace halvoe
{
  enum class SerialMessageType : uint8_t
  {
    invalid = 0,
    data,
    command
  };

  enum class SerialDataCode : uint16_t
  {
    invalid = 0,
    fileList
  };

  enum class SerialCommandCode : uint16_t
  {
    invalid = 0,
    playFile,
    endPlayback
  };

  // This is the type, which holds the size in bytes of the strings, contained in the serial message.
  // "The type of the size, which is transfered over the 'Serial' wire."
  using SerialStringSizeType = uint16_t;
  // This is the type, which holds the size in bytes of the serial message.
  // "The type of the size, which is transfered over the 'Serial' wire."
  using SerialMessageSizeType = uint32_t;

  template<size_t tc_serializerBufferSize,
           size_t tc_deserializerBufferSize>
  class SerialInterface
  {
    public:
      static const uint16_t c_interfaceTag = 0x5A47;
      static const uint16_t c_interfaceVersion = 0x0001;

      struct OutMessage
      {
        friend SerialInterface<tc_serializerBufferSize, tc_deserializerBufferSize>;

        public:
          Serializer<tc_serializerBufferSize> m_serializer;
        
        private:
          SerializerReference<SerialMessageSizeType> m_messageSize;

        public:
          OutMessage(Serializer<tc_serializerBufferSize>& in_serializer, SerializerReference<SerialMessageSizeType>& in_messageSize) :
            m_serializer(in_serializer), m_messageSize(in_messageSize)
          {}
      };

    protected:
      HardwareSerial& m_serial;
      std::array<uint8_t, tc_serializerBufferSize> m_serializerBuffer;
      std::array<uint8_t, tc_deserializerBufferSize> m_deserializerBuffer;

    private:
      virtual bool doHandleData(Deserializer<tc_deserializerBufferSize>& io_deserializer, SerialDataCode in_code) = 0;
      virtual bool doHandleCommand(Deserializer<tc_deserializerBufferSize>& io_deserializer, SerialCommandCode in_code) = 0;

      bool handleData(Deserializer<tc_deserializerBufferSize>& io_deserializer)
      {
        auto code = io_deserializer.template readEnum<SerialDataCode>();
        return doHandleData(io_deserializer, code);
      }

      bool handleCommand(Deserializer<tc_deserializerBufferSize>& io_deserializer)
      {
        auto code = io_deserializer.template readEnum<SerialCommandCode>();
        return doHandleCommand(io_deserializer, code);
      }
      
    public:
      SerialInterface(HardwareSerial& in_serial) : m_serial(in_serial)
      {}

      // Implementer has to config and "begin" Serial port.
      virtual bool setup() = 0;

      OutMessage beginMessage(SerialMessageType in_messageType)
      {
        LOG_TRACE("beginMessage()");
        Serializer<tc_serializerBufferSize> serialzer(m_serializerBuffer);
        auto messageSize = serialzer.template skip<SerialMessageSizeType>();
        serialzer.template write<uint16_t>(c_interfaceTag);
        serialzer.template write<uint16_t>(c_interfaceVersion);
        serialzer.template writeEnum<SerialMessageType>(in_messageType);
        return OutMessage(serialzer, messageSize);
      }
      
      bool sendMessage(OutMessage& io_message)
      {
        LOG_TRACE("sendMessage()");
        io_message.m_messageSize.write(io_message.m_serializer.getBytesWritten() - sizeof(SerialMessageSizeType)); // this <- should not be the problem
        return m_serial.write(m_serializerBuffer.data(), io_message.m_serializer.getBytesWritten()) == io_message.m_serializer.getBytesWritten();
      }

      bool receiveMessage()
      {
        if (m_serial.available() < sizeof(SerialMessageSizeType)) { return false; }
        LOG_TRACE("m_serial.available() < sizeof(SerialMessageSizeType)");
        
        Deserializer<tc_deserializerBufferSize> deserializer(m_deserializerBuffer);
        // sizeof(SerialMessageSizeType) corresponds to the number of bytes used to store the message size in bytes
        m_serial.readBytes(m_deserializerBuffer.data(), sizeof(SerialMessageSizeType));
        SerialMessageSizeType messageSize = deserializer.template read<SerialMessageSizeType>();
        LOG_DEBUG(messageSize);
        size_t bytesReceived = m_serial.readBytes(m_deserializerBuffer.data() + deserializer.getBytesRead(), messageSize);
        LOG_DEBUG(bytesReceived);
        
        if (bytesReceived != messageSize) { return false; }

        auto interfaceTag = deserializer.template read<uint16_t>();
        LOG_DEBUG(interfaceTag, HEX);// ToDo: Check tag!
        auto interfaceVersion = deserializer.template read<uint16_t>();
        LOG_DEBUG(interfaceVersion, HEX);// ToDo: Check version!
        auto serialMessageType = deserializer.template readEnum<SerialMessageType>();
        LOG_DEBUG(static_cast<std::underlying_type<SerialMessageType>::type>(serialMessageType));

        switch (serialMessageType)
        {
          case SerialMessageType::invalid: return false;
          case SerialMessageType::data: return handleData(deserializer);
          case SerialMessageType::command: return handleCommand(deserializer);
        }

        return true;
      }
  };
}
