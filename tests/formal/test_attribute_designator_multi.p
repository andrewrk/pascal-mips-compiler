program testAttributeDesignatorMulti;

class room

BEGIN
   VAR doors  : integer;
      windows : integer;
END	      


class house

BEGIN
   VAR users	 : integer;
      livingroom : room;
      garage	 : room;
    function house; begin
        livingroom := new room;
        garage := new room;
    end
END		 


class testAttributeDesignatorMulti

BEGIN
   
   VAR renters : integer;
       my      : house;
       yours   : house;

FUNCTION testAttributeDesignatorMulti;
BEGIN
   renters := 6;
   my := new house;
   yours := new house;

   my.users := 1;
   my.garage.doors := 3;
   my.garage.windows := 1;
   PRINT my.garage.windows;

   yours.users := renters;
   yours.livingroom.doors := my.garage.doors;
   yours.livingroom.windows := my.garage.windows;
   
   PRINT yours.livingroom.doors;
   PRINT yours.livingroom.windows;
   PRINT yours.users
END

END
.
