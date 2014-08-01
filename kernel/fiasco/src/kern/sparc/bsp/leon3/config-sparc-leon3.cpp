INTERFACE [sparc && leon3]:

#define TARGET_NAME "LEON3"

EXTENSION class Config
{
public:
  enum
  {
    Num_register_windows = 8,
    Stack_frame_size     = 96,
  };
};
