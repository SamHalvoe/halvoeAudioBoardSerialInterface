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
    private:
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
      SerialInterface() = default;

      // Implementer has to config and "begin" Serial port.
      virtual bool setup() = 0;

      void send()
      {
        Serializer<tc_serializerBufferSize> serialzer(m_serializerBuffer);

      }

      bool receive()
      {
        if (Serial1.available() < sizeof(SerialMessageSizeType)) { return false; }
        
        Deserializer<tc_deserializerBufferSize> deserializer(m_deserializerBuffer);
        Serial1.readBytes(m_deserializerBuffer.data(), sizeof(SerialMessageSizeType));
        SerialMessageSizeType messageSize = deserializer.template read<SerialMessageSizeType>();
        size_t bytesReceived = Serial1.readBytes(m_deserializerBuffer.data() + deserializer.getBytesRead(),
                                                 messageSize - deserializer.getBytesRead());
        
        if (bytesReceived != messageSize) { return false; }

        auto interfaceTag = deserializer.template read<uint16_t>();
        // ToDo: Check tag!
        auto interfaceVersion = deserializer.template read<uint16_t>();
        // ToDo: Check version!
        auto serialMessageType = SerialMessageType{ deserializer.template read<std::underlying_type<SerialMessageType>::type>() };

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
