public class Floor {
	
	private int floorNumber = 0;
    private	Appartment[] appartments;
	private static final int DEFAUIT_APPARTMENT_CAPACITY = 4;

	public Floor(int floorNumber, int countAppartment, NumberGenerator generator){
		this.floorNumber = floorNumber;
	    appartments = new Appartment[countAppartment];
		//creating apps on the floor
		for(int i = 0; i < countAppartment; i++){
				appartments[i] = new Appartment(generator.getNext(),DEFAUIT_APPARTMENT_CAPACITY);
		}
	}
	
	public String toString(){
		String result = "Floor number" + floorNumber + "\n";
		for(Appartment app : appartments){
			result+= app.toString() +"\n"; 
		}
		return result;
	}
	//======================================
	
	public Appartment getFreeAppartment(){
		for(Appartment appartment : appartments){
			if(appartment.isFree()){				
				return appartment;
			}
		}
		return null;
	}
}//class