#pragma once

#include <BasicSerializer.hpp>

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
    playFile
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
        public:
          Serializer<tc_serializerBufferSize> m_serializer;
          SerializerReference<SerialMessageSizeType> m_messageSize;

        public:
          OutMessage(Serializer<tc_serializerBufferSize>&& in_serializer, SerializerReference<SerialMessageSizeType>&& in_messageSize) :
            m_serializer(std::move(in_serializer)), m_messageSize(std::move(in_messageSize))
          {}
      };

    private:
      HardwareSerial& m_serial;
      std::array<uint8_t, tc_serializerBufferSize> m_serializerBuffer;
      std::array<uint8_t, tc_deserializerBufferSize> m_deserializerBuffer;

    private:
      virtual bool doHandleData(Deserializer<tc_deserializerBufferSize>&& in_deserializer, SerialDataCode in_code) = 0;
      virtual bool doHandleCommand(Deserializer<tc_deserializerBufferSize>&& in_deserializer, SerialCommandCode in_code) = 0;

      bool handleData(Deserializer<tc_deserializerBufferSize>&& in_deserializer)
      {
        auto code = SerialDataCode{ in_deserializer.template read<std::underlying_type<SerialDataCode>::type>() };
        return doHandleData(std::move(in_deserializer), code);
      }

      bool handleCommand(Deserializer<tc_deserializerBufferSize>&& in_deserializer)
      {
        auto code = SerialCommandCode{ in_deserializer.template read<std::underlying_type<SerialCommandCode>::type>() };
        return doHandleCommand(std::move(in_deserializer), code);
      }
      
    public:
      SerialInterface(HardwareSerial& in_serial) : m_serial(in_serial)
      {}

      // Implementer has to config and "begin" Serial port.
      virtual bool setup() = 0;

      OutMessage beginMessage(SerialMessageType in_messageType)
      {
        Serializer<tc_serializerBufferSize> serialzer(m_serializerBuffer);
        auto messageSize = serialzer.template skip<SerialMessageSizeType>();
        serialzer.template write<uint16_t>(c_interfaceTag);
        serialzer.template write<uint16_t>(c_interfaceVersion);
        serialzer.template writeEnum<SerialMessageType>(in_messageType);
        return OutMessage(std::move(serialzer), std::move(messageSize));
      }
      
      size_t sendMessage(OutMessage&& in_message)
      {
        in_message.m_messageSize.write(in_message.m_serializer.template getBytesWritten() - sizeof(SerialMessageSizeType));
        return m_serial.write(m_serializerBuffer.data(), in_message.m_serializer.template getBytesWritten());
      }

      bool receiveMessage()
      {
        if (m_serial.available() < sizeof(SerialMessageSizeType)) { return false; }
        
        Deserializer<tc_deserializerBufferSize> deserializer(m_deserializerBuffer);
        m_serial.readBytes(m_deserializerBuffer.data(), sizeof(SerialMessageSizeType));
        SerialMessageSizeType messageSize = deserializer.template read<SerialMessageSizeType>();
        size_t bytesReceived = m_serial.readBytes(m_deserializerBuffer.data() + deserializer.getBytesRead(),
                                                  messageSize - deserializer.getBytesRead());
        
        if (bytesReceived != messageSize) { return false; }

        auto interfaceTag = deserializer.template read<uint16_t>();
        // ToDo: Check tag!
        auto interfaceVersion = deserializer.template read<uint16_t>();
        // ToDo: Check version!
        auto serialMessageType = deserializer.template readEnum<SerialMessageType>();

        switch (serialMessageType)
        {
          case SerialMessageType::invalid: return false;
          case SerialMessageType::data: return handleData(std::move(deserializer));
          case SerialMessageType::command: return handleCommand(std::move(deserializer));
        }

        return true;
      }
  };
}
