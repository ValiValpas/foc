//INTERFACE [sparc && leon3]:
INTERFACE [sparc]:

#define TARGET_NAME "LEON3"

EXTENSION class Config
{
public:
  enum
  {
    Num_register_windows = 8,
  };
};
