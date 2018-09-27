public class Floor {
	private int floorNumber =1;
	private Appartment appartments[];
	
	public Floor(int floorNumber, int appCount, NmbrGener genNmbr){
		this.floorNumber = floorNumber;
		
		appartments = new Appartment[appCount];
		for(int i =0;i<appCount;i++){
				appartments[i] = new Appartment(genNmbr.getNext());
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