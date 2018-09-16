public class Floor {
	
	private int floorNumber = 0;
    private	Appartment[] appartments;

	public Floor(int floorNumber, int countAppartment, NumberGenerator generator){
		this.floorNumber = floorNumber;
	    appartments = new Appartment[countAppartment];
		//creating apps on the floor
		for(int i = 0; i < countAppartment; i++){
				appartments[i] = new Appartment(generator.getNext());
		}
	}
	
	public Appartment getFreeAppartment(){
		
		/*for(Appartment appartment : appartments){
			if(appartment.isFree()){				
				//break;
				return appartment;
			}
		}
		return appartment;
	*/
	 return null;
	}
	
	public String toString(){
		String result = "Floor number" + floorNumber + "\n";
		for(Appartment app : appartments){
			result+= app.toString() +"\n"; 
		}
		return result;
	}
}//class