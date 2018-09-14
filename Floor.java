public class Floor {
	
	private int floorNumber = 0;
	Appartment[] appartments;

	public Floor(int floorNumber, int countAppartment){
		this.floorNumber = floorNumber;
	    appartments = new Appartment[countAppartment];
		
		//creating apps on the floor
		for(int i = 0; i < countAppartment; i++){
				appartments[i] = new Appartment(i+1);
		}
		
	}
	
	public Appartment getFreeAppartment(){
		
		for(Appartment appartment : appartments){
			if(appartment.isFree()){				
				//break;
				return appartment;
			}
		}
		return appartment;
	}
	
	public String toString(){
		String result = "Floor" + floorNumber;
		return result;
	}
}//class