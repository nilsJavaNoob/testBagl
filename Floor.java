public class Floor {
	private int floorNumber;
	private Appartment appartments[];
	
	public Floor(int floorNumber, int appCount,NumberGenerator nmbGen){
		System.out.println("Creatingfloor - " + floorNumber);
		this.floorNumber = floorNumber;
		
		appartments = new Appartment[appCount];
		for(int i =0;i<appCount;i++){

				appartments[i] = new Appartment(nmbGen.getNext());
		}
	}
	
	public  String toString(){
		String result = "\n|| Floor " + floorNumber + " ||\n";
		for(Appartment app : appartments){
			result += app.toString()+ "\n"; 
		}
		return result;
	}
}//class