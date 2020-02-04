led_distance = 8 * 2.54;
wall_thickness = 1;
roof_thickness= 0.3;

direction = -1; // albo -1
led_half_angle = 60;
led_height = 12;
spot_radius = led_distance/2 * sqrt(2);
wall_height = led_height + spot_radius /tan(led_half_angle); 


inside_height = wall_height;
total_height = roof_thickness + wall_height;
inside_width = led_distance;
total_width = inside_width + wall_thickness * 2;

difference()
{
    cube( [total_width, total_width,  total_height], center=true);

    translate( [0, 0, abs(direction)/direction * roof_thickness] )
    {
        color( "red" )
        {
        cube( [inside_width, inside_width, total_height], center=true);
        }
    }
}
